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

test('setDiscoveryFilter with uuids', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  t.execution(() => adapter.setDiscoveryFilter({ uuids: ['0000180a-0000-1000-8000-00805f9b34fb'] }))
})

test('setDiscoveryFilter with rssi', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  t.execution(() => adapter.setDiscoveryFilter({ rssi: -70 }))
})

test('setDiscoveryFilter with transport', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  t.execution(() => adapter.setDiscoveryFilter({ transport: 'le' }))
})

test('stopDiscovery', { skip: isCI }, (t) => {
  using adapter = new Adapter()
  adapter.startDiscovery()

  t.execution(() => adapter.stopDiscovery())
})

test('discovery emits device event', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.ok(device)
  t.ok(adapter.devices.has(device.path))
})

test('enumerate returns the devices map', { skip: isCI }, (t) => {
  using adapter = new Adapter()

  const devices = adapter.enumerate()

  t.is(devices, adapter.devices)
})

test('enumerate surfaces known devices without discovery', { skip: isCI }, (t) => {
  using adapter = new Adapter()

  const seen = []
  adapter.on('device', (device) => seen.push(device))

  t.is(adapter.devices.size, 0, 'empty before enumerate')

  adapter.enumerate()

  // Requires at least one paired device on the test machine; BlueZ re-creates
  // those at boot, so InterfacesAdded never fires for them.
  if (adapter.devices.size === 0) {
    t.comment('no known devices, skipping')
    return
  }

  t.is(seen.length, adapter.devices.size, 'emits device for each')

  for (const device of adapter.devices.values()) {
    t.ok(typeof device.address === 'string')
    t.ok(adapter.devices.has(device.path))
  }
})

test('enumerate is idempotent', { skip: isCI }, (t) => {
  using adapter = new Adapter()

  adapter.enumerate()
  const size = adapter.devices.size

  if (size === 0) {
    t.comment('no known devices, skipping')
    return
  }

  let emitted = 0
  adapter.on('device', () => emitted++)

  adapter.enumerate()

  t.is(adapter.devices.size, size, 'no new devices')
  t.is(emitted, 0, 'no duplicate events')
})

test('enumerate populates the cached gatt tree', { skip: isCI }, (t) => {
  using adapter = new Adapter()

  adapter.enumerate()

  let withServices = 0
  for (const device of adapter.devices.values()) {
    if (device.services.size > 0) withServices++
  }

  if (withServices === 0) {
    t.comment('no device has a cached gatt tree, skipping')
    return
  }

  t.pass(`${withServices} device(s) have cached services`)
})

test('getDevice finds an enumerated device by address', { skip: isCI }, (t) => {
  using adapter = new Adapter()

  adapter.enumerate()

  const [device] = adapter.devices.values()

  if (!device) {
    t.comment('no known devices, skipping')
    return
  }

  t.is(adapter.getDevice(device.address), device)
  t.is(adapter.getDevice(device.address.toLowerCase()), device, 'case insensitive')
})

test('getDevice returns null for an unknown address', { skip: isCI }, (t) => {
  using adapter = new Adapter()

  adapter.enumerate()

  t.is(adapter.getDevice('00:00:00:00:00:00'), null)
})
