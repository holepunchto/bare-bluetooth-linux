const binding = require('../binding')

module.exports = exports = class Characteristic {
  constructor(adapter, path, uuid) {
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

  read() {
    if (this._adapter._destroyed) return
    return binding.charRead(this._adapter._handle, this._path)
  }

  write(value) {
    if (this._adapter._destroyed) return
    binding.charWrite(this._adapter._handle, this._path, value)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Characteristic },
      uuid: this.uuid,
      path: this.path
    }
  }
}
