const { test, hook } = require('brittle')
const { Adapter, Characteristic } = require('..')
const { isCI } = require('./helpers')

let adapter
let device
let service
let characteristic
let writableCharacteristic

function needsCharacteristic(t) {
  if (!characteristic) {
    t.pass('no characteristic available')
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

  characteristic = chars.notify
  writableCharacteristic = chars.write
})

test('characteristic is an instance of Characteristic', { skip: isCI }, (t) => {
  if (!needsCharacteristic(t)) return
  t.ok(characteristic instanceof Characteristic)
})

test('characteristic has uuid', { skip: isCI }, (t) => {
  if (!needsCharacteristic(t)) return
  t.ok(typeof characteristic.uuid === 'string')
  t.ok(characteristic.uuid.length > 0)
})

test('characteristic has path', { skip: isCI }, (t) => {
  if (!needsCharacteristic(t)) return
  t.ok(typeof characteristic.path === 'string')
  t.ok(characteristic.path.length > 0)
})

test('service tracks characteristics', { skip: isCI }, (t) => {
  if (!service) return t.pass('no service available')
  t.ok(service.characteristics.size > 0)
})

test('characteristic has flags', { skip: isCI }, (t) => {
  if (!needsCharacteristic(t)) return
  t.ok(Array.isArray(characteristic.flags))
})

test('read returns a buffer', { skip: isCI }, async (t) => {
  if (!needsCharacteristic(t)) return
  const data = await characteristic.read()
  t.ok(data instanceof ArrayBuffer)
})

test('startNotify enables notifications', { skip: isCI }, async (t) => {
  if (!needsCharacteristic(t)) return
  await characteristic.startNotify()
  t.pass()
})

test('data event receives a buffer', { skip: isCI, timeout: 10000 }, async (t) => {
  if (!needsCharacteristic(t)) return

  await characteristic.startNotify()

  const data = await new Promise((resolve) => {
    const timeout = setTimeout(() => resolve(null), 5000)
    characteristic.once('data', (buf) => {
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
  if (!needsCharacteristic(t)) return
  await characteristic.stopNotify()
  t.pass()
})

hook('teardown', { skip: isCI }, async (t) => {
  if (device) await device.disconnect()
  if (adapter) adapter.destroy()
})
