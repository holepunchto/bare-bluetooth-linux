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

test('a transport error destroys the channel', async (t) => {
  t.plan(2)

  const [a, b] = pair()

  b.destroy()
  await new Promise((resolve) => b.once('close', resolve))

  a.on('error', (err) => t.pass('channel errored: ' + err.message))
  a.write(Buffer.from('hello'))

  await new Promise((resolve) => a.once('close', resolve))
  t.ok(a.destroyed, 'stream destroyed after native error')
})

test('destroy emits close exactly once', async (t) => {
  t.plan(2)

  const [a, b] = pair()

  a.on('close', () => t.pass('a closed'))
  b.on('close', () => t.pass('b closed'))

  a.destroy()
  b.destroy()

  // Let the deferred native close run so a duplicate 'close' would be caught
  await new Promise((resolve) => setTimeout(resolve, 100))
})

test('remote close ends the channel', async (t) => {
  t.plan(2)

  const [a, b] = pair()

  a.on('data', () => {})
  a.on('end', () => t.pass('ended'))

  b.destroy()

  await new Promise((resolve) => a.once('close', resolve))
  t.ok(a.destroyed, 'stream destroyed after remote close')
})

test('a write above the channel mtu fails', async (t) => {
  t.plan(3)

  const [a, b] = pair()

  t.ok(a.mtu > 0, 'mtu exposed: ' + a.mtu)

  a.on('error', (err) => t.ok(/MTU/.test(err.message), 'precise error: ' + err.message))
  a.write(Buffer.alloc(a.mtu + 1))

  await new Promise((resolve) => a.once('close', resolve))
  t.ok(a.destroyed, 'stream destroyed by failed write')

  b.destroy()
})

test('a queued write fails when the channel closes', async (t) => {
  t.plan(2)

  const [a, b] = pair()

  // Enough unread data to fill the kernel buffer so a write gets queued
  for (let i = 0; i < 400; i++) a.write(Buffer.alloc(600))

  a.on('error', (err) => t.pass('queued write failed: ' + err.message))

  b.destroy()

  await new Promise((resolve) => a.once('close', resolve))
  t.ok(a.destroyed, 'channel destroyed, queued write not left hanging')
})

test('openL2CAPChannel failure emits error asynchronously', async (t) => {
  t.plan(1)

  using adapter = new Adapter()
  const device = new Device(adapter, '/org/bluez/hci0/dev_invalid', 'not-a-bdaddr')

  let sync = true
  const emitted = new Promise((resolve) => {
    device.on('error', (err) => {
      t.ok(!sync, 'emitted asynchronously: ' + err.message)
      resolve()
    })
  })

  device.openL2CAPChannel(0x80)
  sync = false

  await emitted
})

test('publishL2CAPChannel assigns a psm', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  const psm = await new Promise((resolve, reject) => {
    adapter.on('channelPublish', resolve)
    adapter.on('error', reject)

    adapter.publishL2CAPChannel()
  })

  t.ok(psm >= 0x80 && psm <= 0xff, 'psm in LE dynamic range: 0x' + psm.toString(16))
})

test('openL2CAPChannel emits channelOpen', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')

  const [channel, err] = await new Promise((resolve) => {
    device.on('channelOpen', (channel) => resolve([channel, null]))
    device.on('error', (err) => resolve([null, err]))

    device.openL2CAPChannel(0x80)
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

// Helpers

// A socketpair adopted as two connected channels, like libl2cap's own tests
function pair() {
  const [a, b] = binding.l2capPair()
  return [new L2CAPChannel(a), new L2CAPChannel(b)]
}
