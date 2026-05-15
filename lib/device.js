const binding = require('../binding')

module.exports = exports = class Device {
  constructor(adapter, path, address) {
    this._adapter = adapter
    this._path = path
    this._address = address
  }

  get path() {
    return this._path
  }

  get address() {
    return this._address
  }

  get name() {
    if (this._adapter._destroyed) return undefined
    return binding.deviceGetName(this._adapter._handle, this._path)
  }

  get rssi() {
    if (this._adapter._destroyed) return undefined
    return binding.deviceGetRSSI(this._adapter._handle, this._path)
  }

  get paired() {
    if (this._adapter._destroyed) return undefined
    return binding.deviceGetPaired(this._adapter._handle, this._path)
  }

  get connected() {
    if (this._adapter._destroyed) return undefined
    return binding.deviceGetConnected(this._adapter._handle, this._path)
  }

  connect() {
    if (this._adapter._destroyed) return undefined
    binding.deviceConnect(this._adapter._handle, this._path)
  }

  disconnect() {
    if (this._adapter._destroyed) return undefined
    binding.deviceDisconnect(this._adapter._handle, this._path)
  }

  pair() {
    if (this._adapter._destroyed) return undefined
    binding.devicePair(this._adapter._handle, this._path)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Device },
      path: this._path,
      address: this._address,
      name: this.name
    }
  }
}
