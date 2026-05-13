const test = require('brittle')
const { Adapter } = require('..')
const { isCI } = require('./helpers')

test('constructor sets default path', (t) => {
  using adapter = new Adapter()
  t.is(adapter.path, '/org/bluez/hci0')
})

test('constructor accepts custom path', (t) => {
  using adapter = new Adapter({ path: '/org/bluez/hci1' })
  t.is(adapter.path, '/org/bluez/hci1')
})

test('powered is a boolean', (t) => {
  using adapter = new Adapter()
  t.is(typeof adapter.powered, 'boolean')
})

test('discovering is a boolean', (t) => {
  using adapter = new Adapter()
  t.is(typeof adapter.discovering, 'boolean')
})

test('address is a MAC string or null', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  t.ok(typeof adapter.address === 'string')
})

test('set powered toggles value', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  const before = adapter.powered

  adapter.powered = !before
  t.is(adapter.powered, !before)

  adapter.powered = before
  t.is(adapter.powered, before)
})

test('destroy is idempotent', (t) => {
  const adapter = new Adapter()
  t.execution(() => {
    adapter.destroy()
    adapter.destroy()
  })
})

test('inspect shape', (t) => {
  using adapter = new Adapter()
  const obj = adapter[Symbol.for('bare.inspect')]()
  t.ok('path' in obj)
  t.ok('powered' in obj)
  t.ok('discovering' in obj)
})

test('devices map starts empty', (t) => {
  using adapter = new Adapter()
  t.is(adapter.devices.size, 0)
})

test('is an EventEmitter', (t) => {
  using adapter = new Adapter()
  t.is(typeof adapter.on, 'function')
  t.is(typeof adapter.emit, 'function')
})

test('startDiscovery', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  t.execution(() => adapter.startDiscovery())
})

test('stopDiscovery', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  adapter.startDiscovery()

  t.execution(() => adapter.stopDiscovery())
})

test('discovery emits device with expected shape', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.ok(typeof device.address === 'string')
  t.ok(device.address.length > 0)
  t.ok(typeof device.path === 'string')
  t.ok(device.name === null || typeof device.name === 'string')
  t.ok(typeof device.rssi === 'number')
  t.ok(typeof device.paired === 'boolean')
  t.ok(typeof device.connected === 'boolean')
})

test('discovered device is tracked in devices map', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.ok(adapter.devices.has(device.path))
  t.is(adapter.devices.get(device.path), device)
})

test('destroy cleans up after discovery', { skip: isCI, timeout: 10000 }, async (t) => {
  const adapter = new Adapter()

  adapter.startDiscovery()

  await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  t.execution(() => adapter.destroy())
})
