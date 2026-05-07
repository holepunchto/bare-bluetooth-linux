const test = require('brittle')
const { Adapter } = require('..')
const { isCI } = require('./helpers')

test('constructor sets default path', (t) => {
  const adapter = new Adapter()
  t.is(adapter.path, '/org/bluez/hci0')
  adapter.destroy()
})

test('constructor accepts custom path', (t) => {
  const adapter = new Adapter({ path: '/org/bluez/hci1' })
  t.is(adapter.path, '/org/bluez/hci1')
  adapter.destroy()
})

test('powered is a boolean', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  t.is(typeof adapter.powered, 'boolean')
  adapter.destroy()
})

test('discovering is a boolean', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  t.is(typeof adapter.discovering, 'boolean')
  adapter.destroy()
})

test('address is a MAC string', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  t.ok(/^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/.test(adapter.address))
  adapter.destroy()
})

test('set powered toggles value', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  const before = adapter.powered

  adapter.powered = !before
  t.is(adapter.powered, !before)

  adapter.powered = before
  t.is(adapter.powered, before)

  adapter.destroy()
})

test('destroy is idempotent', (t) => {
  const adapter = new Adapter()
  t.execution(() => {
    adapter.destroy()
    adapter.destroy()
  })
})

test('using disposes', (t) => {
  t.execution(() => {
    using adapter = new Adapter()
    t.ok(adapter.path)
  })
})

test('inspect shape', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  const obj = adapter[Symbol.for('bare.inspect')]()
  t.ok('path' in obj)
  t.ok('powered' in obj)
  adapter.destroy()
})
