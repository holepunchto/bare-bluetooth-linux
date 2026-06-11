module.exports = exports = class GattApplication {
  constructor() {
    this._services = []
  }

  get services() {
    return this._services
  }

  addService(service) {
    this._services.push(service)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: GattApplication },
      services: this.services
    }
  }
}
