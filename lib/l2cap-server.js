const binding = require('../binding')

// Wraps a native listening-server handle: owns it and knows how to release it
module.exports = exports = class L2CAPServer {
  constructor(handle) {
    this._handle = handle
  }

  get psm() {
    if (!this._handle) return 0
    return binding.l2capServerPsm(this._handle)
  }

  unpublish() {
    if (!this._handle) return
    binding.l2capUnpublish(this._handle)
    this._handle = null
  }
}
