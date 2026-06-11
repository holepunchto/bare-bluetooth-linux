const test = require('brittle')
const { Adapter, Advertisement } = require('..')
const { isCI } = require('./helpers')

test('Advertisement is exported', (t) => {
  t.is(typeof Advertisement, 'function')
  t.is(Advertisement.name, 'Advertisement')
})

test('advertisement has default properties', (t) => {
  const ad = new Advertisement()
  t.is(ad.type, 'peripheral')
  t.is(ad.localName, undefined)
  t.alike(ad.serviceUUIDs, [])
})

test('advertisement accepts options', (t) => {
  const ad = new Advertisement({
    type: 'broadcast',
    localName: 'TestDevice',
    serviceUUIDs: ['180a', '180d']
  })
  t.is(ad.type, 'broadcast')
  t.is(ad.localName, 'TestDevice')
  t.alike(ad.serviceUUIDs, ['180a', '180d'])
})

test('register and unregister advertisement', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  const ad = new Advertisement({
    type: 'peripheral',
    localName: 'BareTest',
    serviceUUIDs: ['180a']
  })

  await adapter.registerAdvertisement(ad)
  t.pass('advertisement registered')

  await adapter.unregisterAdvertisement(ad)
  t.pass('advertisement unregistered')
})
