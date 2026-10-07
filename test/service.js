const test = require('brittle')
const { Adapter, Service } = require('..')
const { isCI, findDevice } = require('./helpers')

test('device services after connect', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  const device = await findDevice(adapter)

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')

  try {
    await device.connect()
  } catch (err) {
    t.comment('connect error: ' + err.message)
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

  await device.disconnect()
})

test(
  'services of a device connected before attaching are serviceCached',
  { skip: isCI, timeout: 60000 },
  async (t) => {
    using adapter = new Adapter()

    const device = await findDevice(adapter)

    try {
      await device.connect()
    } catch (err) {
      t.pass('connect failed (device may not support it)')
      return
    }

    if (!device.servicesResolved) {
      await new Promise((resolve) => device.once('servicesResolved', resolve))
    }

    // Attaching while the link is up: bluetoothd replays device and services
    using late = new Adapter()

    const cached = await new Promise((resolve) => {
      late.on('deviceCached', (d) => {
        if (d.address === device.address) d.on('serviceCached', resolve)
      })
    })

    t.ok(cached instanceof Service)
    t.ok(device.services.has(cached.path), cached.uuid)

    await device.disconnect()
  }
)
