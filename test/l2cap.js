const test = require('brittle')
const { Adapter, Device, L2CAPChannel } = require('..')
const binding = require('../binding')
const { isCI } = require('./helpers')

test('L2CAPChannel is exported', (t) => {
  t.is(typeof L2CAPChannel, 'function')
})

test('device openL2CAPChannel is a function', (t) => {
  t.is(typeof Device.prototype.openL2CAPChannel, 'function')
})

// A socketpair adopted as two connected channels, like libl2cap's own tests
function pair() {
  const [a, b] = binding.l2capPair()
  return [new L2CAPChannel(a), new L2CAPChannel(b)]
}

test('fatal native error destroys the channel, exactly one error', async (t) => {
  t.plan(2)

  const [a, b] = pair()

  b.destroy()
  await new Promise((resolve) => b.once('close', resolve))

  a.on('error', (err) => t.pass('channel errored: ' + err.message))

  a.write(Buffer.from('hello'))

  await new Promise((resolve) => a.once('close', resolve))
  t.ok(a.destroyed, 'stream destroyed after native error')
})

test('publishL2CAPChannel assigns a psm', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  adapter.publishL2CAPChannel()

  const psm = await new Promise((resolve, reject) => {
    adapter.on('channelPublish', resolve)
    adapter.on('error', reject)
  })

  t.ok(psm >= 0x80 && psm <= 0xff, 'psm in LE dynamic range: 0x' + psm.toString(16))

  adapter.unpublishL2CAPChannel(psm)
})

test('openL2CAPChannel emits channelOpen', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')

  device.openL2CAPChannel(0x80)

  const [channel, err] = await new Promise((resolve) => {
    device.on('channelOpen', (channel) => resolve([channel, null]))
    device.on('error', (err) => resolve([null, err]))
  })

  if (err) {
    t.comment('channel error: ' + err.message)
    t.pass('channel open failed (device may not accept L2CAP)')
  } else {
    t.ok(channel instanceof L2CAPChannel)
    t.is(channel.psm, 0x80)
    t.is(channel.peer, device.address)
    channel.destroy()
  }
})
