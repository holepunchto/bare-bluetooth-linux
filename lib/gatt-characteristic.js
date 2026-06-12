const EventEmitter = require('bare-events')

module.exports = exports = class GattCharacteristic extends EventEmitter {
  constructor({ uuid, flags = [], value = new Uint8Array() } = {}) {
    super()
    this._uuid = uuid
    this._flags = flags
    this._value = value
  }

  get uuid() {
    return this._uuid
  }

  get flags() {
    return this._flags
  }

  get value() {
    return this._value
  }

  set value(v) {
    this._value = v
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: GattCharacteristic },
      uuid: this.uuid,
      flags: this.flags
    }
  }
}
