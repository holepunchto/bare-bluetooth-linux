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
  t.ok(typeof device.rssi === 'number')
  t.ok(typeof device.paired === 'boolean')
  t.ok(typeof device.connected === 'boolean')
})
