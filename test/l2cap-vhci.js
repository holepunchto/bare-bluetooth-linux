const test = require('brittle')
const { Adapter, Advertisement } = require('..')
const { vhci } = require('./helpers')

function teardown(t, client, server) {
  t.teardown(() => {
    client.destroy()
    server.destroy()
  })
}

async function openPair(t, { accept = true, serverSecurity, clientSecurity } = {}) {
  const server = new Adapter({ path: vhci.a })
  const client = new Adapter({ path: vhci.b })
  teardown(t, client, server)

  server.powered = true
  client.powered = true

  server.publishL2CAPChannel({ security: serverSecurity })
  const psm = await new Promise((resolve, reject) => {
    server.on('channelPublish', resolve)
    server.on('error', reject)
  })

  const ad = new Advertisement({ type: 'peripheral', localName: 'bare-vhci' })
  await server.registerAdvertisement(ad)

  client.setDiscoveryFilter({ transport: 'le' })
  client.startDiscovery()
  let device = [...client.devices.values()].find((d) => d.address === server.address)
  if (!device) {
    device = await new Promise((resolve) => {
      const ondevice = (d) => {
        if (d.address !== server.address) return
        client.off('device', ondevice)
        resolve(d)
      }
      client.on('device', ondevice)
    })
  }
  client.stopDiscovery()

  if (!accept) return { server, client, device, psm }

  const inbound = new Promise((resolve) => server.on('channelOpen', resolve))

  device.openL2CAPChannel(psm, { security: clientSecurity })
  const outbound = await new Promise((resolve, reject) => {
    device.on('channelOpen', resolve)
    device.on('error', reject)
  })

  return { server, client, device, psm, inbound: await inbound, outbound }
}

test('l2cap end to end over virtual controllers', { skip: !vhci, timeout: 30000 }, async (t) => {
  const { psm, inbound, outbound } = await openPair(t)

  t.ok(psm > 0, 'psm published: 0x' + psm.toString(16))

  const fromServer = new Promise((resolve) => outbound.once('data', resolve))
  inbound.write(Buffer.from('ping'))
  t.alike(Buffer.from(await fromServer), Buffer.from('ping'), 'accepted side writes')

  const fromClient = new Promise((resolve) => inbound.once('data', resolve))
  outbound.write(Buffer.from('pong'))
  t.alike(Buffer.from(await fromClient), Buffer.from('pong'), 'accepted side reads')

  outbound.destroy()
  inbound.destroy()
})

test('adapter destroy closes accepted channels', { skip: !vhci, timeout: 30000 }, async (t) => {
  const { server, inbound } = await openPair(t)

  const closed = new Promise((resolve) => inbound.once('close', resolve))
  server.destroy()

  await closed
  t.ok(inbound.destroyed, 'accepted channel destroyed with the adapter')
})

test('unhandled inbound connection is closed', { skip: !vhci, timeout: 30000 }, async (t) => {
  const { device, psm } = await openPair(t, { accept: false })

  // Depending on timing the peer teardown surfaces as a short-lived channel
  // or as a failed open; either proves the server did not keep it
  const result = await new Promise((resolve) => {
    device.on('channelOpen', (channel) => {
      channel.resume()
      channel.once('close', () => resolve('closed'))
    })
    device.on('error', () => resolve('rejected'))

    device.openL2CAPChannel(psm)
  })

  t.ok(result === 'closed' || result === 'rejected', 'unhandled connection torn down: ' + result)
})

// The rig never bonds the two controllers, which is what makes the two sides
// behave differently below.
async function openUnpaired(t, opts) {
  const { device, psm } = await openPair(t, { ...opts, accept: false })

  return new Promise((resolve) => {
    device.on('channelOpen', (channel) => {
      channel.destroy()
      resolve('opened')
    })
    device.on('error', () => resolve('refused'))

    device.openL2CAPChannel(psm, { security: opts.clientSecurity })
  })
}

// l2cap_core.c l2cap_le_connect_req() answers L2CAP_CR_LE_ENCRYPTION (0x0008)
// when smp_sufficient_security() fails, with no path to accepting anyway.
test(
  'a server requiring encryption refuses an unbonded peer',
  { skip: !vhci, timeout: 30000 },
  async (t) => {
    const result = await openUnpaired(t, { serverSecurity: 'medium' })
    t.is(result, 'refused', 'server security kept the unbonded peer out')
  }
)

// Asymmetric on purpose. With no bond there is no stored LTK, so smp.c
// smp_conn_security() logs "security requested but not available" and returns
// 1, which l2cap_le_start() reads as "nothing to wait for" and sends the
// connect request in the clear. Bonded peers take the smp_ltk_encrypt() path
// above it and are encrypted.
test(
  'client security does not gate an unbonded link',
  { skip: !vhci, timeout: 30000 },
  async (t) => {
    const result = await openUnpaired(t, { clientSecurity: 'medium' })
    t.is(result, 'opened', 'kernel connected in the clear despite the request')
  }
)

test('a low security level still connects', { skip: !vhci, timeout: 30000 }, async (t) => {
  const { outbound } = await openPair(t, { serverSecurity: 'low', clientSecurity: 'low' })

  t.ok(!outbound.destroyed, 'low is the kernel default and changes nothing')
  outbound.destroy()
})

test(
  'destroy in the same tick as publish releases the psm',
  { skip: !vhci, timeout: 30000 },
  async (t) => {
    const adapter = new Adapter({ path: vhci.a })
    adapter.powered = true

    adapter.on('channelPublish', () => t.fail('emitted on a destroyed adapter'))
    adapter.publishL2CAPChannel({ psm: 0x81 })
    adapter.destroy()

    // Let the deferred registration run
    await new Promise((resolve) => setTimeout(resolve, 100))

    // The psm must be free again: a fresh adapter can bind it
    using retry = new Adapter({ path: vhci.a })
    retry.publishL2CAPChannel({ psm: 0x81 })
    const psm = await new Promise((resolve, reject) => {
      retry.on('channelPublish', resolve)
      retry.on('error', reject)
    })
    t.is(psm, 0x81, 'psm rebindable after same-tick destroy')
  }
)

// Needs the gc hook: bare --expose-gc node_modules/.bin/brittle-bare <file>
test(
  'connect callback does not pin the device',
  { skip: !vhci || typeof globalThis.gc !== 'function', timeout: 30000 },
  async (t) => {
    let { client, device, outbound } = await openPair(t)

    const ref = new WeakRef(device)
    device = null

    client.destroy() // drops the adapter's device map

    await new Promise((resolve) => setTimeout(resolve, 100))
    globalThis.gc()
    await new Promise((resolve) => setTimeout(resolve, 100))
    globalThis.gc()

    t.is(ref.deref(), undefined, 'device collected while the channel lives on')
    t.ok(!outbound.destroyed, 'channel still alive without it')

    outbound.destroy()
  }
)
