const { test, hook } = require('brittle')
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
  t.ok(device.addressType === 'public' || device.addressType === 'random')
  t.ok(typeof device.path === 'string')
  t.ok(device.name === undefined || typeof device.name === 'string')
  t.ok(device.rssi === undefined || typeof device.rssi === 'number')
  t.ok(typeof device.paired === 'boolean')
  t.ok(typeof device.connected === 'boolean')
  t.ok(Array.isArray(device.uuids))
  t.ok(typeof device.manufacturerData === 'object')
  t.ok(typeof device.serviceData === 'object')
  t.ok(typeof device.servicesResolved === 'boolean')
})

test('device connect', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')

  try {
    await device.connect()
  } catch (err) {
    t.comment('connect error: ' + err.message)
    t.pass('connect failed (device may not support it)')
    return
  }

  t.comment('connected: ' + device.connected)
  t.is(device.connected, true)

  try {
    await device.disconnect()
    t.comment('disconnected: ' + device.connected)
  } catch (err) {
    t.comment('disconnect error: ' + err.message)
  }
})

test('device pair', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')
  t.comment('paired before: ' + device.paired)

  try {
    await device.pair()
    t.comment('paired after: ' + device.paired)
    t.is(device.paired, true)
  } catch (err) {
    t.comment('pair error: ' + err.message)
    t.pass('pair failed (device may not support it)')
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

let eventAdapter
let eventDevice

function needsDevice(t) {
  if (!eventDevice) {
    t.pass('no device available')
    return false
  }
  return true
}

hook('setup', { skip: isCI, timeout: 60000 }, async (t) => {
  eventAdapter = new Adapter()
  eventAdapter.startDiscovery()

  eventDevice = await new Promise((resolve) => {
    eventAdapter.on('device', resolve)
  })

  eventAdapter.stopDiscovery()
})

test('connected event on connect', { skip: isCI, timeout: 60000 }, async (t) => {
  if (!needsDevice(t)) return

  const connected = new Promise((resolve) => {
    eventDevice.once('connected', resolve)
  })

  try {
    await eventDevice.connect()
  } catch (err) {
    t.comment('connect error: ' + err.message)
    return t.pass('connect failed')
  }

  t.is(await connected, true)
})

test('connected event on disconnect', { skip: isCI, timeout: 60000 }, async (t) => {
  if (!needsDevice(t)) return
  if (!eventDevice.connected) return t.pass('device not connected')

  const disconnected = new Promise((resolve) => {
    eventDevice.once('connected', resolve)
  })

  await eventDevice.disconnect()

  t.is(await disconnected, false)
})

hook('teardown', { skip: isCI }, async (t) => {
  if (eventDevice && eventDevice.connected) await eventDevice.disconnect()
  if (eventAdapter) eventAdapter.destroy()
})
