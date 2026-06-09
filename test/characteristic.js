const { test, hook } = require('brittle')
const { Adapter, Characteristic } = require('..')
const { isCI } = require('./helpers')

let adapter
let device
let service
let notifiableCharacteristic
let writableCharacteristic

function needsNotifiableCharacteristic(t) {
  if (!notifiableCharacteristic) {
    t.pass('no notifiable characteristic available')
    return false
  }
  return true
}

function needsWritableCharacteristic(t) {
  if (!writableCharacteristic) {
    t.pass('no writable characteristic available')
    return false
  }
  return true
}

hook('setup', { skip: isCI, timeout: 60000 }, async (t) => {
  adapter = new Adapter()
  adapter.startDiscovery()

  device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

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

  if (!service) return

  const chars = await new Promise((resolve) => {
    const found = { notify: null, write: null }
    const timeout = setTimeout(() => resolve(found), 10000)
    service.on('characteristic', (c) => {
      if (!found.notify && c.flags.includes('notify')) found.notify = c
      if (!found.write && c.flags.includes('write')) found.write = c
      if (found.notify && found.write) {
        clearTimeout(timeout)
        resolve(found)
      }
    })
  })

  notifiableCharacteristic = chars.notify
  writableCharacteristic = chars.write
})

test('characteristic is an instance of Characteristic', { skip: isCI }, (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  t.ok(notifiableCharacteristic instanceof Characteristic)
})

test('characteristic has uuid', { skip: isCI }, (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  t.ok(typeof notifiableCharacteristic.uuid === 'string')
  t.ok(notifiableCharacteristic.uuid.length > 0)
})

test('characteristic has path', { skip: isCI }, (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  t.ok(typeof notifiableCharacteristic.path === 'string')
  t.ok(notifiableCharacteristic.path.length > 0)
})

test('service tracks characteristics', { skip: isCI }, (t) => {
  if (!service) return t.pass('no service available')
  t.ok(service.characteristics.size > 0)
})

test('characteristic has flags', { skip: isCI }, (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  t.ok(Array.isArray(notifiableCharacteristic.flags))
})

test('mtu is a number', { skip: isCI }, (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  t.ok(typeof notifiableCharacteristic.mtu === 'number')
  t.ok(notifiableCharacteristic.mtu > 0)
})

test('read returns a buffer', { skip: isCI }, async (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  const data = await notifiableCharacteristic.read()
  t.ok(data instanceof ArrayBuffer)
})

test('startNotify enables notifications', { skip: isCI }, async (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  await notifiableCharacteristic.startNotify()
  t.pass()
})

test('data event receives a buffer', { skip: isCI, timeout: 10000 }, async (t) => {
  if (!needsNotifiableCharacteristic(t)) return

  await notifiableCharacteristic.startNotify()

  const data = await new Promise((resolve) => {
    const timeout = setTimeout(() => resolve(null), 5000)
    notifiableCharacteristic.once('data', (buf) => {
      clearTimeout(timeout)
      resolve(buf)
    })
  })

  t.ok(data instanceof ArrayBuffer)
})

test('write resolves', { skip: isCI }, async (t) => {
  if (!needsWritableCharacteristic(t)) return
  await writableCharacteristic.write(new Uint8Array([0x01]))
  t.pass()
})

test('write with type request resolves', { skip: isCI }, async (t) => {
  if (!needsWritableCharacteristic(t)) return
  await writableCharacteristic.write(new Uint8Array([0x01]), { type: 'request' })
  t.pass()
})

test('write with type command resolves', { skip: isCI }, async (t) => {
  if (!needsWritableCharacteristic(t)) return
  await writableCharacteristic.write(new Uint8Array([0x01]), { type: 'command' })
  t.pass()
})

test('stopNotify disables notifications', { skip: isCI }, async (t) => {
  if (!needsNotifiableCharacteristic(t)) return
  await notifiableCharacteristic.stopNotify()
  t.pass()
})

hook('teardown', { skip: isCI }, async (t) => {
  if (device) await device.disconnect()
  if (adapter) adapter.destroy()
})
