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

test('set powered surfaces a BlueZ error', (t) => {
  using adapter = new Adapter({ path: '/org/bluez/hci9' })

  t.exception(() => {
    adapter.powered = true
  })
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
  })

  await t.exception(() => adapter.setDiscoveryFilter({ rssi: -70 }), /destroyed/)
  await t.exception(() => adapter.startDiscovery(), /destroyed/)
  await t.exception(() => adapter.stopDiscovery(), /destroyed/)
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

test('startDiscovery', { skip: isCI }, async (t) => {
  using adapter = new Adapter()
  await t.execution(() => adapter.startDiscovery())
})

test('setDiscoveryFilter with uuids', { skip: isCI }, async (t) => {
  using adapter = new Adapter()
  await t.execution(() =>
    adapter.setDiscoveryFilter({ uuids: ['0000180a-0000-1000-8000-00805f9b34fb'] })
  )
})

test('setDiscoveryFilter with rssi', { skip: isCI }, async (t) => {
  using adapter = new Adapter()
  await t.execution(() => adapter.setDiscoveryFilter({ rssi: -70 }))
})

test('setDiscoveryFilter with transport', { skip: isCI }, async (t) => {
  using adapter = new Adapter()
  await t.execution(() => adapter.setDiscoveryFilter({ transport: 'le' }))
})

test('a uuid nobody advertises reports no device', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  await new Promise((resolve) => setTimeout(resolve, 200))

  let reported = 0
  adapter.on('device', (device) => {
    reported++
    device.on('rssi', () => reported++)
  })
  for (const device of adapter.devices.values()) device.on('rssi', () => reported++)

  await adapter.setDiscoveryFilter({ uuids: ['0000dead-0000-1000-8000-00805f9b34fb'] })
  await adapter.startDiscovery()
  await new Promise((resolve) => setTimeout(resolve, 3000))
  await adapter.stopDiscovery()

  t.is(reported, 0, 'nothing advertises that uuid, so nothing is reported')
})

test('stopDiscovery', { skip: isCI }, async (t) => {
  using adapter = new Adapter()
  await adapter.startDiscovery()

  await t.execution(() => adapter.stopDiscovery())
})

test('discovery reports a device it hears', { skip: isCI, timeout: 20000 }, async (t) => {
  using adapter = new Adapter()

  const heard = new Promise((resolve) => {
    adapter.on('device', (device) => device.once('rssi', (rssi) => resolve({ device, rssi })))
  })

  await adapter.startDiscovery()

  const { device, rssi } = await heard

  await adapter.stopDiscovery()

  t.is(typeof rssi, 'number', 'rssi: ' + rssi)
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

  await adapter.startDiscovery()
  t.is(await started, true, 'discovery reported as started')

  const stopped = new Promise((resolve) => adapter.once('discovering', resolve))

  await adapter.stopDiscovery()
  t.is(await stopped, false, 'discovery reported as stopped')
})
