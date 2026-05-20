module.exports = exports = class Service {
  constructor(device, path, uuid, primary) {
    this._device = device
    this._path = path
    this._uuid = uuid
    this._primary = primary
  }

  get path() {
    return this._path
  }

  get uuid() {
    return this._uuid
  }

  get primary() {
    return this._primary
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Service },
      uuid: this._uuid,
      primary: this._primary
    }
  }
}
