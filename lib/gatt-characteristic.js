const EventEmitter = require('bare-events')
const binding = require('../binding')

const GATT_ERRORS = new Set([
  'Failed',
  'InProgress',
  'NotPermitted',
  'NotAuthorized',
  'NotSupported',
  'InvalidOffset',
  'InvalidValueLength'
])

function toDBusError(err) {
  const code = err && err.code
  const name = GATT_ERRORS.has(code) ? code : 'Failed'
  const message = err instanceof Error ? err.message : String(err)
  return ['org.bluez.Error.' + name, message]
}

module.exports = exports = class GattCharacteristic extends EventEmitter {
  constructor({ uuid, flags = [], value = new Uint8Array(), read = null, write = null } = {}) {
    super()
    this._uuid = uuid
    this._flags = flags
    this._value = value
    this._read = read
    this._write = write
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

  set read(fn) {
    this._read = fn
  }

  set write(fn) {
    this._write = fn
  }

  async _onread(id, options) {
    let value

    try {
      value = this._read === null ? this._value : await this._read(options)
    } catch (err) {
      if (this._unregistered) return
      binding.gattRespondError(this._adapter._handle, id, ...toDBusError(err))
      return
    }

    if (this._unregistered) return

    binding.gattCharacteristicRespondRead(
      this._adapter._handle,
      id,
      value instanceof Uint8Array ? value : new Uint8Array(value)
    )
  }

  async _onwrite(id, value, options) {
    try {
      if (this._write !== null) await this._write(value, options)
    } catch (err) {
      if (this._unregistered) return
      binding.gattRespondError(this._adapter._handle, id, ...toDBusError(err))
      return
    }

    if (this._unregistered) return

    this._value = value
    binding.gattCharacteristicRespondWrite(this._adapter._handle, id, value)
    this.emit('write', value, options)
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
