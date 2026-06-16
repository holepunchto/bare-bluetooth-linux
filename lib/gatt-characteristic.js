const EventEmitter = require('bare-events')
const binding = require('../binding')

module.exports = exports = class GattCharacteristic extends EventEmitter {
  constructor({ uuid, flags = [], value = new Uint8Array() } = {}) {
    super()
    this._uuid = uuid
    this._flags = flags
    this._value = value
    this._adapter = null
    this._serviceIndex = -1
    this._characteristicIndex = -1
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

  _reset() {
    this._adapter = null
    this._serviceIndex = -1
    this._characteristicIndex = -1
  }

  set value(v) {
    this._value = v
    if (this._adapter && this._serviceIndex >= 0 && this._characteristicIndex >= 0) {
      binding.gattCharacteristicSetValue(
        this._adapter._handle,
        this._serviceIndex,
        this._characteristicIndex,
        v instanceof Uint8Array ? v : new Uint8Array(v)
      )
    }
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: GattCharacteristic },
      uuid: this.uuid,
      flags: this.flags
    }
  }
}
