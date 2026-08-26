// End-to-end L2CAP over two virtual controllers (test/vhci.sh). Public API
// only: publish on one adapter, discover and connect from the other.
const test = require('brittle')
const { Adapter, Advertisement } = require('..')
const { vhci } = require('./helpers')

test('l2cap end to end over virtual controllers', { skip: !vhci, timeout: 30000 }, async (t) => {
  using server = new Adapter({ path: vhci.a })
  using client = new Adapter({ path: vhci.b })

  server.powered = true
  client.powered = true

  const serverAddress = server.address

  server.publishL2CAPChannel()
  const psm = await new Promise((resolve, reject) => {
    server.on('channelPublish', resolve)
    server.on('error', reject)
  })
  t.ok(psm > 0, 'psm published: 0x' + psm.toString(16))

  const ad = new Advertisement({ type: 'peripheral', localName: 'bare-vhci' })
  await server.registerAdvertisement(ad)

  client.setDiscoveryFilter({ transport: 'le' })
  client.startDiscovery()
  const device = await new Promise((resolve) => {
    client.on('device', (d) => {
      if (d.address === serverAddress) resolve(d)
    })
  })
  client.stopDiscovery()
  t.pass('server discovered at ' + device.address)

  device.openL2CAPChannel(psm)
  const [inbound, outbound] = await Promise.all([
    new Promise((resolve) => server.on('channelOpen', resolve)),
    new Promise((resolve, reject) => {
      device.on('channelOpen', resolve)
      device.on('error', reject)
    })
  ])
  t.pass('channel open on both sides')

  const fromServer = new Promise((resolve) => outbound.once('data', resolve))
  inbound.write(Buffer.from('ping'))
  t.alike(Buffer.from(await fromServer), Buffer.from('ping'), 'accepted side writes')

  const fromClient = new Promise((resolve) => inbound.once('data', resolve))
  outbound.write(Buffer.from('pong'))
  t.alike(Buffer.from(await fromClient), Buffer.from('pong'), 'accepted side reads')

  outbound.destroy()
  inbound.destroy()

  await server.unregisterAdvertisement(ad)
})
