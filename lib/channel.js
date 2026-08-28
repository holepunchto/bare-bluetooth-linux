const { Duplex } = require('bare-stream')

const binding = require('../binding')
const errors = require('./errors')

module.exports = exports = class L2CAPChannel extends Duplex {
  constructor(channelHandle) {
    super()

    this._writeCallback = null

    this._handle = binding.l2capInit(
      channelHandle,
      this,
      this._ondata,
      this._ondrain,
      this._onend,
      this._onerror,
      this._onclose,
      this._onopen
    )
  }

  get psm() {
    if (!this._handle) return 0
    return binding.l2capPsm(this._handle)
  }

  get peer() {
    if (!this._handle) return null
    return binding.l2capPeer(this._handle)
  }

  get mtu() {
    if (!this._handle) return 0
    return binding.l2capMtu(this._handle)
  }

  _open(cb) {
    binding.l2capOpen(this._handle)
    cb()
  }

  _write(chunk, _encoding, cb) {
    if (chunk.byteLength === 0) return cb(null)

    if (chunk.byteLength > this.mtu) {
      return cb(
        errors.WRITE_FAILED(`Write of ${chunk.byteLength} bytes exceeds channel MTU of ${this.mtu}`)
      )
    }

    const res = binding.l2capWrite(this._handle, chunk)
    if (res === 0) {
      cb(null)
    } else if (res === 1) {
      // Queued: hold the callback so the Duplex waits for the drain
      this._writeCallback = cb
    } else {
      cb(errors.WRITE_FAILED('Write failed'))
    }
  }

  _destroy(err, cb) {
    if (this._handle) binding.l2capEnd(this._handle)
    cb(err)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: L2CAPChannel },
      destroyed: this.destroyed
    }
  }

  _ondata(data) {
    this.push(data)
  }

  _ondrain() {
    const cb = this._writeCallback
    this._writeCallback = null

    this.emit('drain')

    if (cb) cb(null)
  }

  _onend() {
    this.push(null)
  }

  _onerror(message) {
    this.destroy(new Error(message))
  }

  _onclose() {
    this._handle = null

    // A queued write can never complete once the native side is closed;
    // failing its callback is what lets the stream finish destroying
    const cb = this._writeCallback
    this._writeCallback = null
    if (cb) cb(errors.WRITE_FAILED('Channel closed'))

    this.destroy()
  }

  _onopen() {
    this.emit('open')
  }
}
