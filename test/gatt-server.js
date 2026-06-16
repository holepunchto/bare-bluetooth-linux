const test = require('brittle')
const { Adapter, GattApplication, GattService, GattCharacteristic } = require('..')
const { isCI } = require('./helpers')

test('GattApplication is exported', (t) => {
  t.is(typeof GattApplication, 'function')
  t.is(GattApplication.name, 'GattApplication')
})

test('GattService is exported', (t) => {
  t.is(typeof GattService, 'function')
  t.is(GattService.name, 'GattService')
})

test('GattCharacteristic is exported', (t) => {
  t.is(typeof GattCharacteristic, 'function')
  t.is(GattCharacteristic.name, 'GattCharacteristic')
})

test('GattApplication manages services', (t) => {
  const app = new GattApplication({ path: '/com/test/gatt' })
  t.is(app.path, '/com/test/gatt')
  t.alike(app.services, [])

  const svc = new GattService({ uuid: '180a' })
  app.addService(svc)
  t.is(app.services.length, 1)
  t.is(app.services[0], svc)
})

test('GattService has default properties', (t) => {
  const svc = new GattService({ uuid: '180d' })
  t.is(svc.uuid, '180d')
  t.is(svc.primary, true)
  t.alike(svc.characteristics, [])
})

test('GattService accepts primary = false', (t) => {
  const svc = new GattService({ uuid: '180d', primary: false })
  t.is(svc.primary, false)
})

test('GattService manages characteristics', (t) => {
  const svc = new GattService({ uuid: '180d' })
  const ch = new GattCharacteristic({ uuid: '2a37', flags: ['read', 'notify'] })
  svc.addCharacteristic(ch)
  t.is(svc.characteristics.length, 1)
  t.is(svc.characteristics[0], ch)
})

test('GattCharacteristic has default properties', (t) => {
  const ch = new GattCharacteristic({ uuid: '2a37' })
  t.is(ch.uuid, '2a37')
  t.alike(ch.flags, [])
  t.ok(ch.value instanceof Uint8Array)
  t.is(ch.value.length, 0)
})

test('GattCharacteristic accepts flags and value', (t) => {
  const value = new Uint8Array([0x01, 0x02])
  const ch = new GattCharacteristic({
    uuid: '2a37',
    flags: ['read', 'write'],
    value
  })
  t.alike(ch.flags, ['read', 'write'])
  t.alike(ch.value, value)
})

test('GattCharacteristic value setter', (t) => {
  const ch = new GattCharacteristic({ uuid: '2a37' })
  const newValue = new Uint8Array([0xaa, 0xbb])
  ch.value = newValue
  t.alike(ch.value, newValue)
})

test('full GATT tree assembly', (t) => {
  const app = new GattApplication({ path: '/com/test/gatt' })

  const svc = new GattService({ uuid: '12345678-1234-1234-1234-123456789abc' })
  const ch1 = new GattCharacteristic({
    uuid: '12345678-1234-1234-1234-123456789ab1',
    flags: ['read']
  })
  const ch2 = new GattCharacteristic({
    uuid: '12345678-1234-1234-1234-123456789ab2',
    flags: ['write']
  })
  svc.addCharacteristic(ch1)
  svc.addCharacteristic(ch2)
  app.addService(svc)

  t.is(app.services.length, 1)
  t.is(app.services[0].characteristics.length, 2)
  t.is(app.services[0].characteristics[0].uuid, '12345678-1234-1234-1234-123456789ab1')
  t.is(app.services[0].characteristics[1].uuid, '12345678-1234-1234-1234-123456789ab2')
})

test('registerApplication rejects if already registered', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  const app = new GattApplication({ path: '/com/test/gatt' })
  const svc = new GattService({ uuid: '12345678-1234-1234-1234-123456789abc' })
  const ch = new GattCharacteristic({
    uuid: '12345678-1234-1234-1234-123456789ab1',
    flags: ['read']
  })
  svc.addCharacteristic(ch)
  app.addService(svc)

  await adapter.registerApplication(app)

  const app2 = new GattApplication({ path: '/com/test/gatt2' })
  app2.addService(new GattService({ uuid: '12345678-1234-1234-1234-123456789abd' }))

  await t.exception(() => adapter.registerApplication(app2), /already registered/)

  await adapter.unregisterApplication(app)
})

test('characteristic value setter throws after unregister', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  const app = new GattApplication({ path: '/com/test/gatt' })
  const svc = new GattService({ uuid: '12345678-1234-1234-1234-123456789abc' })
  const ch = new GattCharacteristic({
    uuid: '12345678-1234-1234-1234-123456789ab1',
    flags: ['read', 'write']
  })
  svc.addCharacteristic(ch)
  app.addService(svc)

  await adapter.registerApplication(app)
  await adapter.unregisterApplication(app)

  t.exception(() => {
    ch.value = new Uint8Array([0x01])
  })
})
