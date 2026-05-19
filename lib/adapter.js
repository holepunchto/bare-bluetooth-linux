const EventEmitter = require('bare-events')
const binding = require('../binding')
const Device = require('./device')

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
      this._onmethodreply
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
      path: this._path,
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

  _onmethodreply() {}
}
