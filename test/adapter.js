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
