const binding = require('../binding')

module.exports = exports = class Service {
  constructor(device, path, uuid) {
    this._device = device
    this._path = path
    this._uuid = uuid
  }

  get path() {
    return this._path
  }

  get uuid() {
    return this._uuid
  }

  get primary() {
    if (this._device._adapter._destroyed) return
    return binding.serviceIsPrimary(this._device._adapter._handle, this._path)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Service },
      uuid: this.uuid,
      primary: this.primary
    }
  }
}
