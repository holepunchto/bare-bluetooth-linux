const binding = require('../binding')

module.exports = exports = class Adapter {
  constructor(opts = {}) {
    this._path = opts.path || '/org/bluez/hci0'
    this._destroyed = false

    this._handle = binding.adapterInit(this._path)
  }

  get path() {
    return this._path
  }

  get powered() {
    return binding.adapterGetPowered(this._handle)
  }

  set powered(val) {
    binding.adapterSetPowered(this._handle, val)
  }

  get discovering() {
    return binding.adapterGetDiscovering(this._handle)
  }

  get address() {
    return binding.adapterGetAddress(this._handle)
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true
    binding.adapterDestroy(this._handle)
  }

  [Symbol.dispose]() {
    this.destroy()
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Adapter },
      path: this._path,
      powered: this.powered
    }
  }
}
