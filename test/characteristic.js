const { test, hook } = require('brittle')
const { Adapter, Characteristic } = require('..')
const { isCI } = require('./helpers')

let adapter
let device
let service
let characteristic

function needsCharacteristic(t) {
  if (!characteristic) {
    t.pass('no characteristic available')
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

  const connectErr = await new Promise((resolve) => device.connect(resolve))
  if (connectErr) {
    t.comment('connect error: ' + connectErr.message)
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

  characteristic = await new Promise((resolve) => {
    const timeout = setTimeout(() => resolve(null), 10000)
    service.on('characteristic', (c) => {
      if (c.flags.includes('notify')) {
        clearTimeout(timeout)
        resolve(c)
      }
    })
  })
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

test('read returns a buffer', { skip: isCI }, (t) => {
  if (!needsCharacteristic(t)) return
  const data = characteristic.read()
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

test('stopNotify disables notifications', { skip: isCI }, async (t) => {
  if (!needsCharacteristic(t)) return
  await characteristic.stopNotify()
  t.pass()
})

hook('teardown', { skip: isCI }, async (t) => {
  if (device) await new Promise((resolve) => device.disconnect(resolve))
  if (adapter) adapter.destroy()
})
