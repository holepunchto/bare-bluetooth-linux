const EventEmitter = require('bare-events')
const binding = require('../binding')

// Error names BlueZ accepts on a ReadValue reply, each mapped to an ATT error
// code on its side. Anything else becomes Failed
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

  // BlueZ is waiting on a reply here, so a rejected read goes back as a D-Bus
  // error rather than up to a caller. Set err.code to one of the org.bluez.Error
  // names (NotPermitted, NotAuthorized, ...) to pick the ATT error the central sees;
  // a Failed whose message is '0x80'..'0x9f' passes that application code through
  async _onread(id, options) {
    let value

    try {
      value = this._read === null ? this._value : await this._read(options)
    } catch (err) {
      if (this._unregistered) return
      binding.gattCharacteristicRespondReadError(this._adapter._handle, id, ...toDBusError(err))
      return
    }

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
