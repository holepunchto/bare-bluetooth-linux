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

test('accessors after destroy do not reach a closed connection', async (t) => {
  const adapter = new Adapter()
  adapter.destroy()

  // The teardown chain closes the D-Bus connections a few loop turns later
  await new Promise((resolve) => setTimeout(resolve, 500))

  t.is(adapter.powered, undefined, 'powered')
  t.is(adapter.discovering, undefined, 'discovering')
  t.is(adapter.address, undefined, 'address')

  t.execution(() => {
    adapter.powered = true
    adapter.setDiscoveryFilter({ rssi: -70 })
    adapter.startDiscovery()
    adapter.stopDiscovery()
  })
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

test('powering the adapter emits powered', { skip: isCI, timeout: 10000 }, async (t) => {
  // Not `using`: disposal runs before t.teardown, and restoring power needs a
  // live adapter
  const adapter = new Adapter()

  const wasPowered = adapter.powered
  t.teardown(() => {
    adapter.powered = wasPowered
    adapter.destroy()
  })

  const toggled = new Promise((resolve) => adapter.once('powered', resolve))

  adapter.powered = !wasPowered

  t.is(await toggled, !wasPowered, 'BlueZ signalled the change')
})

test('discovery emits discovering', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  const started = new Promise((resolve) => adapter.once('discovering', resolve))

  adapter.startDiscovery()
  t.is(await started, true, 'discovery reported as started')

  const stopped = new Promise((resolve) => adapter.once('discovering', resolve))

  adapter.stopDiscovery()
  t.is(await stopped, false, 'discovery reported as stopped')
})
