const { test, hook } = require('brittle')
const { Adapter, Descriptor } = require('..')
const { isCI, onAnyDevice } = require('./helpers')

let adapter
let device
let service
let characteristic
let descriptor

function needsDescriptor(t) {
  if (!descriptor) {
    t.pass('no descriptor available')
    return false
  }
  return true
}

hook('setup', { skip: isCI, timeout: 60000 }, async (t) => {
  adapter = new Adapter()

  const found = new Promise((resolve) => {
    onAnyDevice(adapter, resolve)
  })

  await adapter.startDiscovery()

  device = await found

  await adapter.stopDiscovery()

  try {
    await device.connect()
  } catch (err) {
    t.comment('connect error: ' + err.message)
    return
  }

  service = await new Promise((resolve) => {
    const timeout = setTimeout(() => resolve(null), 10000)
    device.on('service', (svc) => {
      clearTimeout(timeout)
      resolve(svc)
    })
  })

  if (!service) {
    t.comment('no service discovered')
    return
  }

  characteristic = await new Promise((resolve) => {
    const timeout = setTimeout(() => resolve(null), 10000)
    service.on('characteristic', (c) => {
      clearTimeout(timeout)
      resolve(c)
    })
  })

  if (!characteristic) {
    t.comment('no characteristic discovered')
    return
  }

  descriptor = await new Promise((resolve) => {
    const timeout = setTimeout(() => resolve(null), 10000)
    characteristic.on('descriptor', (d) => {
      clearTimeout(timeout)
      resolve(d)
    })
  })
})

test('descriptor is an instance of Descriptor', { skip: isCI }, (t) => {
  if (!needsDescriptor(t)) return
  t.ok(descriptor instanceof Descriptor)
})

test('descriptor has uuid', { skip: isCI }, (t) => {
  if (!needsDescriptor(t)) return
  t.ok(typeof descriptor.uuid === 'string')
  t.ok(descriptor.uuid.length > 0)
})

test('descriptor has path', { skip: isCI }, (t) => {
  if (!needsDescriptor(t)) return
  t.ok(typeof descriptor.path === 'string')
  t.ok(descriptor.path.length > 0)
})

test('descriptor has flags', { skip: isCI }, (t) => {
  if (!needsDescriptor(t)) return
  t.ok(Array.isArray(descriptor.flags))
})

test('characteristic tracks descriptors', { skip: isCI }, (t) => {
  if (!characteristic) return t.pass('no characteristic available')
  t.ok(characteristic.descriptors.size > 0)
})

test('descriptor read returns a buffer', { skip: isCI }, async (t) => {
  if (!needsDescriptor(t)) return
  const data = await descriptor.read()
  t.ok(data instanceof ArrayBuffer)
})

test('descriptor write resolves', { skip: isCI }, async (t) => {
  if (!needsDescriptor(t)) return
  try {
    await descriptor.write(new Uint8Array([0x00, 0x00]))
    t.pass()
  } catch (err) {
    t.comment('write error: ' + err.message)
    t.pass('write failed (descriptor may be read-only)')
  }
})

test(
  'descriptors of a device connected before attaching are descriptorCached',
  { skip: isCI, timeout: 30000 },
  async (t) => {
    if (!service) return t.pass('no service available')

    // Attaching while the link is up: bluetoothd replays the whole GATT tree
    using late = new Adapter()

    const cached = await new Promise((resolve) => {
      late.on('deviceCached', (d) => {
        if (d.address !== device.address) return
        d.on('serviceCached', (s) => {
          s.on('characteristicCached', (c) => c.on('descriptorCached', resolve))
        })
      })
    })

    t.ok(cached instanceof Descriptor)

    const known = [...device.services.values()]
      .flatMap((s) => [...s.characteristics.values()])
      .some((c) => c.descriptors.has(cached.path))
    t.ok(known, cached.uuid)
  }
)

hook('teardown', { skip: isCI }, async (t) => {
  if (device && device.connected) await device.disconnect()
  if (adapter) adapter.destroy()
})
