const EventEmitter = require('bare-events')
const binding = require('../binding')

module.exports = exports = class Characteristic extends EventEmitter {
  constructor(adapter, path, uuid) {
    super()
    this._adapter = adapter
    this._path = path
    this._uuid = uuid
  }

  get path() {
    return this._path
  }

  get uuid() {
    return this._uuid
  }

  get flags() {
    if (this._adapter._destroyed) return []
    return binding.charGetFlags(this._adapter._handle, this._path)
  }

  read() {
    if (this._adapter._destroyed) return
    return binding.charRead(this._adapter._handle, this._path)
  }

  write(value) {
    if (this._adapter._destroyed) return
    binding.charWrite(this._adapter._handle, this._path, value)
  }

  startNotify(cb) {
    if (this._adapter._destroyed) return
    binding.charStartNotify(this._adapter._handle, this._path, cb)
  }

  stopNotify(cb) {
    if (this._adapter._destroyed) return
    binding.charStopNotify(this._adapter._handle, this._path, cb)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Characteristic },
      uuid: this.uuid,
      path: this.path
    }
  }
}
