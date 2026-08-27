// End-to-end L2CAP over two virtual controllers (test/vhci.sh). Public API
// only: publish on one adapter, discover and connect from the other.
const test = require('brittle')
const { Adapter, Advertisement } = require('..')
const { vhci } = require('./helpers')

// The publish/advertise/discover/connect dance shared by every scenario
async function openPair(t, { accept = true } = {}) {
  const server = new Adapter({ path: vhci.a })
  const client = new Adapter({ path: vhci.b })
  t.teardown(() => {
    client.destroy()
    server.destroy()
  })

  server.powered = true
  client.powered = true

  server.publishL2CAPChannel()
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
      client.on('device', (d) => {
        if (d.address === server.address) resolve(d)
      })
    })
  }
  client.stopDiscovery()

  if (!accept) return { server, client, device, psm }

  const inbound = new Promise((resolve) => server.on('channelOpen', resolve))

  device.openL2CAPChannel(psm)
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

  device.openL2CAPChannel(psm)

  // Depending on timing the peer teardown surfaces as a short-lived channel
  // or as a failed open; either proves the server did not keep it
  const result = await new Promise((resolve) => {
    device.on('channelOpen', (channel) => {
      channel.resume()
      channel.once('close', () => resolve('closed'))
    })
    device.on('error', () => resolve('rejected'))
  })

  t.ok(result === 'closed' || result === 'rejected', 'unhandled connection torn down: ' + result)
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
