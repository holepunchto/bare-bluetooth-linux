const EventEmitter = require('bare-events')
const binding = require('../binding')
const Device = require('./device')
const Service = require('./service')
const Characteristic = require('./characteristic')

module.exports = exports = class Adapter extends EventEmitter {
  constructor(opts = {}) {
    super()

    this._path = opts.path || '/org/bluez/hci0'
    this._destroyed = false
    this._devices = new Map()

    this._handle = binding.adapterInit(
      this._path,
      this,
      this._ondeviceadded,
      this._ondeviceremoved,
      this._onserviceadded,
      this._onserviceremoved,
      this._oncharadded,
      this._oncharremoved,
      this._oncharvalue,
      this._ondevicepropschanged
    )
  }

  get path() {
    return this._path
  }

  get powered() {
    return binding.adapterGetPowered(this._handle)
  }

  set powered(val) {
    binding.adapterSetPowered(this._handle, val)
  }

  get discovering() {
    return binding.adapterGetDiscovering(this._handle)
  }

  get address() {
    return binding.adapterGetAddress(this._handle)
  }

  get devices() {
    return this._devices
  }

  setDiscoveryFilter({ uuids = [], rssi, transport } = {}) {
    binding.adapterSetDiscoveryFilter(this._handle, uuids, rssi, transport)
  }

  startDiscovery() {
    binding.adapterStartDiscovery(this._handle)
  }

  stopDiscovery() {
    binding.adapterStopDiscovery(this._handle)
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true
    this._devices.clear()
    binding.adapterDestroy(this._handle)
  }

  [Symbol.dispose]() {
    this.destroy()
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Adapter },
      path: this.path,
      powered: this.powered,
      discovering: this.discovering
    }
  }

  _ondeviceadded(path, address) {
    const device = new Device(this, path, address)
    this._devices.set(path, device)
    this.emit('device', device)
  }

  _ondeviceremoved(path) {
    const device = this._devices.get(path)
    if (device) {
      this._devices.delete(path)
      this.emit('deviceRemoved', device)
    }
  }

  _onserviceadded(path, devicePath, uuid) {
    const device = this._devices.get(devicePath)
    if (device) {
      const service = new Service(device, path, uuid)
      device._services.set(path, service)
      device.emit('service', service)
    }
  }

  _onserviceremoved(path, devicePath) {
    const device = this._devices.get(devicePath)
    if (device) {
      const service = device._services.get(path)
      if (service) {
        device._services.delete(path)
        device.emit('serviceRemoved', service)
      }
    }
  }

  _oncharadded(path, servicePath, uuid) {
    for (const device of this._devices.values()) {
      const service = device._services.get(servicePath)
      if (service) {
        const characteristic = new Characteristic(this, path, uuid)
        service._characteristics.set(path, characteristic)
        service.emit('characteristic', characteristic)
        return
      }
    }
  }

  _oncharremoved(path, servicePath) {
    for (const device of this._devices.values()) {
      const service = device._services.get(servicePath)
      if (service) {
        const characteristic = service._characteristics.get(path)
        if (characteristic) {
          service._characteristics.delete(path)
          service.emit('characteristicRemoved', characteristic)
        }
        return
      }
    }
  }

  _ondevicepropschanged(path, connected, paired, servicesResolved, rssi, name) {
    const device = this._devices.get(path)
    if (!device) return

    if (connected !== undefined) device.emit('connected', connected)
    if (paired !== undefined) device.emit('paired', paired)
    if (servicesResolved !== undefined) device.emit('servicesResolved', servicesResolved)
    if (rssi !== undefined) device.emit('rssi', rssi)
    if (name !== undefined) device.emit('name', name)
  }

  _oncharvalue(path, buffer) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        const characteristic = service._characteristics.get(path)
        if (characteristic) {
          characteristic.emit('data', buffer)
          return
        }
      }
    }
  }
}
