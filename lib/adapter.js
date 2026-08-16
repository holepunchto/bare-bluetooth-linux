const EventEmitter = require('bare-events')
const binding = require('../binding')
const Device = require('./device')
const Service = require('./service')
const Characteristic = require('./characteristic')
const Descriptor = require('./descriptor')

module.exports = exports = class Adapter extends EventEmitter {
  constructor(opts = {}) {
    super()

    this._path = opts.path || '/org/bluez/hci0'
    this._destroyed = false
    this._devices = new Map()
    this._gattApp = null

    this._handle = binding.adapterInit(
      this._path,
      this,
      this._ondeviceadded,
      this._ondeviceremoved,
      this._onserviceadded,
      this._onserviceremoved,
      this._oncharadded,
      this._oncharremoved,
      this._ondescadded,
      this._ondescremoved,
      this._oncharvalue,
      this._ondevicepropschanged,
      this._onadvertisementrelease,
      this._ongattcharacteristicwrite
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

  // Registers devices (and their cached GATT tree) that BlueZ already knows about.
  //
  // InterfacesAdded only fires for objects bluetoothd newly creates, so bonded
  // devices are otherwise never surfaced: they are re-created from
  // /var/lib/bluetooth before this process starts. Call this to see them, then
  // connect without scanning.
  //
  // Safe to call repeatedly; objects already registered are skipped, so no
  // duplicate 'device' events are emitted.
  enumerate() {
    const entries = binding.adapterEnumerate(this._handle)
    const parentPath = (path) => path.slice(0, path.lastIndexOf('/'))

    // Apply parent-first: a service whose device is not yet registered would be
    // dropped, and BlueZ's tree order is not contractual.
    for (const kind of ['device', 'service', 'characteristic', 'descriptor']) {
      for (let i = 0; i < entries.length; i += 3) {
        if (entries[i] !== kind) continue

        const path = entries[i + 1]
        const value = entries[i + 2]

        switch (kind) {
          case 'device':
            if (!this._devices.has(path)) this._ondeviceadded(path, value)
            break
          case 'service':
            if (!this._hasService(path)) {
              this._onserviceadded(path, parentPath(path), value)
            }
            break
          case 'characteristic':
            if (!this._hasCharacteristic(path)) {
              this._oncharadded(path, parentPath(path), value)
            }
            break
          case 'descriptor':
            this._ondescadded(path, parentPath(path), value)
            break
        }
      }
    }

    return this._devices
  }

  getDevice(address) {
    const path = this._path + '/dev_' + address.toUpperCase().replace(/:/g, '_')

    return this._devices.get(path) || null
  }

  _hasService(path) {
    for (const device of this._devices.values()) {
      if (device._services.has(path)) return true
    }
    return false
  }

  _hasCharacteristic(path) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        if (service._characteristics.has(path)) return true
      }
    }
    return false
  }

  registerAdvertisement(advertisement) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    return new Promise((resolve, reject) => {
      binding.advertisementRegister(
        this._handle,
        advertisement.type,
        advertisement.serviceUUIDs,
        advertisement.localName,
        (err) => {
          if (err) reject(err)
          else resolve()
        }
      )
    })
  }

  unregisterAdvertisement(advertisement) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    return new Promise((resolve, reject) => {
      binding.advertisementUnregister(this._handle, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  registerApplication(application) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    if (this._gattApp) return Promise.reject(new Error('An application is already registered'))
    this._gattApp = application

    for (let i = 0; i < application.services.length; i++) {
      const svc = application.services[i]
      const svcIdx = binding.gattServiceAdd(this._handle, application.path, svc.uuid, svc.primary)

      for (let j = 0; j < svc.characteristics.length; j++) {
        const ch = svc.characteristics[j]
        ch._path = binding.gattCharacteristicAdd(
          this._handle,
          svcIdx,
          ch.uuid,
          ch.flags,
          ch.value instanceof Uint8Array ? ch.value : new Uint8Array(ch.value)
        )
        ch._adapter = this
      }
    }

    return new Promise((resolve, reject) => {
      binding.gattRegister(this._handle, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  unregisterApplication(application) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    return new Promise((resolve, reject) => {
      binding.gattUnregister(this._handle, (err) => {
        if (err) {
          reject(err)
        } else {
          this._gattApp._reset()
          this._gattApp = null
          resolve()
        }
      })
    })
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true
    this._devices.clear()

    if (this._gattApp) {
      binding.gattUnregister(this._handle, () => {})
      this._gattApp._reset()
      this._gattApp = null
    }

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

  _ondescadded(path, charPath, uuid) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        const characteristic = service._characteristics.get(charPath)
        if (characteristic) {
          const descriptor = new Descriptor(this, path, uuid)
          characteristic._descriptors.set(path, descriptor)
          characteristic.emit('descriptor', descriptor)
          return
        }
      }
    }
  }

  _ondescremoved(path, charPath) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        const characteristic = service._characteristics.get(charPath)
        if (characteristic) {
          const descriptor = characteristic._descriptors.get(path)
          if (descriptor) {
            characteristic._descriptors.delete(path)
            characteristic.emit('descriptorRemoved', descriptor)
          }
          return
        }
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

  _onadvertisementrelease() {
    this.emit('advertisementReleased')
  }

  _ongattcharacteristicwrite(path, value) {
    if (!this._gattApp) return
    for (const svc of this._gattApp.services) {
      for (const ch of svc.characteristics) {
        if (ch._path === path) {
          ch._value = value
          ch.emit('write', value)
          return
        }
      }
    }
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
