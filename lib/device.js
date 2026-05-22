const EventEmitter = require('bare-events')
const binding = require('../binding')

module.exports = exports = class Device extends EventEmitter {
  constructor(adapter, path, address) {
    super()
    this._adapter = adapter
    this.path = path
    this.address = address
    this._services = new Map()
  }

  get services() {
    return this._services
  }

  get path() {
    return this.path
  }

  get address() {
    return this.address
  }

  get name() {
    if (this._adapter._destroyed) return
    return binding.deviceGetName(this._adapter._handle, this.path)
  }

  get rssi() {
    if (this._adapter._destroyed) return
    return binding.deviceGetRSSI(this._adapter._handle, this.path)
  }

  get paired() {
    if (this._adapter._destroyed) return
    return binding.deviceGetPaired(this._adapter._handle, this.path)
  }

  get connected() {
    if (this._adapter._destroyed) return
    return binding.deviceGetConnected(this._adapter._handle, this.path)
  }

  connect(cb) {
    if (this._adapter._destroyed) return
    binding.deviceConnect(this._adapter._handle, this.path, cb)
  }

  disconnect(cb) {
    if (this._adapter._destroyed) return
    binding.deviceDisconnect(this._adapter._handle, this.path, cb)
  }

  pair(cb) {
    if (this._adapter._destroyed) return
    binding.devicePair(this._adapter._handle, this.path, cb)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Device },
      path: this.path,
      address: this.address,
      name: this.name
    }
  }
}
