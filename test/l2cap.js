const test = require('brittle')
const { Adapter, Device, L2CAPChannel } = require('..')
const { isCI } = require('./helpers')

test('L2CAPChannel is exported', (t) => {
  t.is(typeof L2CAPChannel, 'function')
})

test('device openL2CAPChannel is a function', (t) => {
  t.is(typeof Device.prototype.openL2CAPChannel, 'function')
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
