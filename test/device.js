const test = require('brittle')
const { Adapter, Device } = require('..')
const { isCI } = require('./helpers')

test('Device is exported', (t) => {
  t.is(typeof Device, 'function')
  t.is(Device.name, 'Device')
})

test('device inspect shape', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  const obj = device[Symbol.for('bare.inspect')]()
  t.ok('path' in obj)
  t.ok('address' in obj)
  t.ok('name' in obj)
})
