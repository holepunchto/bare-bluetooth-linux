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

test('device connect and disconnect', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.execution(() => device.connect())
  t.is(device.connected, true)

  t.execution(() => device.disconnect())
  t.is(device.connected, false)
})

test('device pair', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.execution(() => device.pair())
  t.is(device.paired, true)
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
