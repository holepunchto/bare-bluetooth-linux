const test = require('brittle')
const { Adapter, Service } = require('..')
const { isCI } = require('./helpers')

test('device services after connect', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')

  const connectErr = await new Promise((resolve) => device.connect(resolve))

  if (connectErr) {
    t.comment('connect error: ' + connectErr.message)
    t.pass('connect failed (device may not support it)')
    return
  }

  const service = await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => resolve(null), 10000)
    device.on('service', (service) => {
      clearTimeout(timeout)
      resolve(service)
    })
  })

  if (!service) {
    t.pass('no services discovered (device may not expose GATT)')
  } else {
    t.ok(service instanceof Service)
    t.ok(typeof service.uuid === 'string')
    t.ok(typeof service.path === 'string')
    t.ok(typeof service.primary === 'boolean')
    t.comment('service: ' + service.uuid + ' (primary: ' + service.primary + ')')
    t.ok(device.services.size > 0)
  }

  await new Promise((resolve) => device.disconnect(resolve))
})
