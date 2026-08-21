const { Duplex } = require('bare-stream')

const binding = require('../binding')
const errors = require('./errors')

module.exports = exports = class L2CAPChannel extends Duplex {
  constructor(channelHandle) {
    super({
      allowHalfOpen: false
    })

    this._channelHandle = channelHandle
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

  _open(cb) {
    binding.l2capOpen(this._handle)
    cb()
  }

  _write(chunk, _encoding, cb) {
    if (chunk.byteLength === 0) return cb(null)

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
    this.emit('error', new Error(message))
  }

  _onclose() {
    this._handle = null

    // The native side discards queued writes on close and never drains them
    const cb = this._writeCallback
    this._writeCallback = null
    if (cb) cb(errors.WRITE_FAILED('Channel closed'))

    this.emit('close')
  }

  _onopen() {
    this.emit('open')
  }
}
