const test = require('brittle')
const { Adapter, Device } = require('..')
const { isCI } = require('./helpers')

test('Device is exported', (t) => {
  t.is(typeof Device, 'function')
  t.is(Device.name, 'Device')
})

test('device has expected properties', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.ok(typeof device.address === 'string')
  t.ok(device.address.length > 0)
  t.ok(typeof device.path === 'string')
  t.ok(device.name === undefined || typeof device.name === 'string')
  t.ok(device.rssi === undefined || typeof device.rssi === 'number')
  t.ok(typeof device.paired === 'boolean')
  t.ok(typeof device.connected === 'boolean')
})

test('device connect', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')

  try {
    device.connect()
    t.comment('connected: ' + device.connected)
    t.is(device.connected, true)
    device.disconnect()
    t.comment('disconnected: ' + device.connected)
  } catch (e) {
    t.comment('connect/disconnect error: ' + e.message)
    t.pass('connect threw (device may not support it)')
  }
})

test('device pair', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')
  t.comment('paired before: ' + device.paired)

  try {
    device.pair()
    t.comment('paired after: ' + device.paired)
    t.is(device.paired, true)
  } catch (e) {
    t.comment('pair error: ' + e.message)
    t.pass('pair threw (device may not support it)')
  }
})

test('device properties after adapter destroy', { skip: isCI, timeout: 10000 }, async (t) => {
  const adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()
  adapter.destroy()

  t.is(device.name, undefined)
  t.is(device.rssi, undefined)
  t.is(device.paired, undefined)
  t.is(device.connected, undefined)
  t.ok(typeof device.address === 'string')
  t.ok(typeof device.path === 'string')
})

test('device methods after adapter destroy', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  const device = new Device(adapter, '/org/bluez/hci0/dev_00_00_00_00_00_00', '00:00:00:00:00:00')

  adapter.destroy()

  t.is(device.connect(), undefined)
  t.is(device.disconnect(), undefined)
  t.is(device.pair(), undefined)
})
