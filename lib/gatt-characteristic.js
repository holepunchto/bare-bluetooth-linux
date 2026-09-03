const EventEmitter = require('bare-events')
const binding = require('../binding')

module.exports = exports = class GattCharacteristic extends EventEmitter {
  constructor({ uuid, flags = [], value = new Uint8Array(), read = null } = {}) {
    super()
    this._uuid = uuid
    this._flags = flags
    this._value = value
    this._read = read
    this._adapter = null
    this._path = null
    this._unregistered = false
    this._notifying = false
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

  get notifying() {
    return this._notifying
  }

  get read() {
    return this._read
  }

  set read(fn) {
    this._read = fn
  }

  async _onread(id) {
    if (this._read === null) return

    const value = await this._read()

    if (this._unregistered) return

    binding.gattCharacteristicRespondRead(
      this._adapter._handle,
      id,
      value instanceof Uint8Array ? value : new Uint8Array(value)
    )
  }

  _reset() {
    this._adapter = null
    this._path = null
    this._unregistered = true
    this._notifying = false
  }

  set value(v) {
    if (this._unregistered) {
      throw new Error('Characteristic is no longer registered')
    }

    this._value = v
    if (this._adapter && this._path) {
      binding.gattCharacteristicSetValue(
        this._adapter._handle,
        this._path,
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
