const binding = require('../binding')

module.exports = exports = class Device {
  constructor(adapterHandle, path, address) {
    this._adapterHandle = adapterHandle
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
    if (!this._adapterHandle) return undefined
    return binding.deviceGetName(this._adapterHandle, this._path)
  }

  get rssi() {
    if (!this._adapterHandle) return undefined
    return binding.deviceGetRSSI(this._adapterHandle, this._path)
  }

  get paired() {
    if (!this._adapterHandle) return undefined
    return binding.deviceGetPaired(this._adapterHandle, this._path)
  }

  get connected() {
    if (!this._adapterHandle) return undefined
    return binding.deviceGetConnected(this._adapterHandle, this._path)
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
