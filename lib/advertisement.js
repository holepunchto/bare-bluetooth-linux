module.exports = exports = class Advertisement {
  constructor({ type = 'peripheral', localName, serviceUUIDs = [], serviceData = {} } = {}) {
    this._type = type
    this._localName = localName
    this._serviceUUIDs = serviceUUIDs
    this._serviceData = serviceData
  }

  get type() {
    return this._type
  }

  get localName() {
    return this._localName
  }

  get serviceUUIDs() {
    return this._serviceUUIDs
  }

  get serviceData() {
    return this._serviceData
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Advertisement },
      type: this.type,
      localName: this.localName,
      serviceUUIDs: this.serviceUUIDs,
      serviceData: this.serviceData
    }
  }
}
