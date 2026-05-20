const test = require('brittle')
const { Adapter, Device, Service } = require('..')
const { isCI } = require('./helpers')

test('Device is exported', (t) => {
  t.is(typeof Device, 'function')
  t.is(Device.name, 'Device')
})

test('device has expected properties', { skip: isCI, timeout: 10000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.ok(typeof device.address === 'string')
  t.ok(device.address.length > 0)
  t.ok(typeof device.path === 'string')
  t.ok(device.name === undefined || typeof device.name === 'string')
  t.ok(device.rssi === undefined || typeof device.rssi === 'number')
  t.ok(typeof device.paired === 'boolean')
  t.ok(typeof device.connected === 'boolean')
})

test('device connect', { skip: isCI, timeout: 60000 }, async (t) => {
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

  t.comment('connected: ' + device.connected)
  t.is(device.connected, true)

  const disconnectErr = await new Promise((resolve) => device.disconnect(resolve))

  if (disconnectErr) {
    t.comment('disconnect error: ' + disconnectErr.message)
  } else {
    t.comment('disconnected: ' + device.connected)
  }
})

test('device pair', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  t.comment('device: ' + device.address + ' (' + (device.name || 'unnamed') + ')')
  t.comment('paired before: ' + device.paired)

  const pairErr = await new Promise((resolve) => device.pair(resolve))

  if (pairErr) {
    t.comment('pair error: ' + pairErr.message)
    t.pass('pair failed (device may not support it)')
  } else {
    t.comment('paired after: ' + device.paired)
    t.is(device.paired, true)
  }
})

test('Service is exported', (t) => {
  t.is(typeof Service, 'function')
  t.is(Service.name, 'Service')
})

test('device discoverServices', { skip: isCI, timeout: 60000 }, async (t) => {
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

  const [err, services] = await new Promise((resolve) => {
    device.discoverServices((err, services) => resolve([err, services]))
  })

  if (err) {
    t.comment('discoverServices error: ' + err.message)
    t.pass('discoverServices failed')
  } else {
    t.ok(Array.isArray(services))
    t.comment('found ' + services.length + ' services')

    for (const service of services) {
      t.ok(service instanceof Service)
      t.ok(typeof service.uuid === 'string')
      t.ok(typeof service.path === 'string')
      t.ok(typeof service.primary === 'boolean')
      t.comment('  service: ' + service.uuid + ' (primary: ' + service.primary + ')')
    }
  }

  await new Promise((resolve) => device.disconnect(resolve))
})

test('device discoverServices with uuid filter', { skip: isCI, timeout: 60000 }, async (t) => {
  using adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()

  const connectErr = await new Promise((resolve) => device.connect(resolve))

  if (connectErr) {
    t.pass('connect failed (device may not support it)')
    return
  }

  const [err, all] = await new Promise((resolve) => {
    device.discoverServices((err, services) => resolve([err, services]))
  })

  if (err || all.length === 0) {
    t.pass('no services to filter')
    await new Promise((resolve) => device.disconnect(resolve))
    return
  }

  const targetUuid = all[0].uuid

  const [err2, filtered] = await new Promise((resolve) => {
    device.discoverServices([targetUuid], (err, services) => resolve([err, services]))
  })

  t.absent(err2)
  t.ok(filtered.length >= 1)
  t.is(filtered[0].uuid, targetUuid)

  await new Promise((resolve) => device.disconnect(resolve))
})

test('device properties after adapter destroy', { skip: isCI, timeout: 10000 }, async (t) => {
  const adapter = new Adapter()

  adapter.startDiscovery()

  const device = await new Promise((resolve) => {
    adapter.on('device', resolve)
  })

  adapter.stopDiscovery()
  adapter.destroy()

  t.is(device.name, undefined)
  t.is(device.rssi, undefined)
  t.is(device.paired, undefined)
  t.is(device.connected, undefined)
  t.ok(typeof device.address === 'string')
  t.ok(typeof device.path === 'string')
})

test('device methods after adapter destroy', { skip: isCI }, (t) => {
  const adapter = new Adapter()
  const device = new Device(adapter, '/org/bluez/hci0/dev_00_00_00_00_00_00', '00:00:00:00:00:00')

  adapter.destroy()

  t.is(device.connect(), undefined)
  t.is(device.disconnect(), undefined)
  t.is(device.pair(), undefined)

  device.discoverServices((err, services) => {
    t.absent(err)
    t.alike(services, [])
  })
})
