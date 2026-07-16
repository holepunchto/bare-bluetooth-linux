const EventEmitter = require('bare-events')
const binding = require('../binding')

const PROPERTY_READ = 0x02
const PROPERTY_WRITE_WITHOUT_RESPONSE = 0x04
const PROPERTY_WRITE = 0x08
const PROPERTY_NOTIFY = 0x10
const PROPERTY_INDICATE = 0x20

module.exports = exports = class Characteristic extends EventEmitter {
  constructor(adapter, path, uuid) {
    super()
    this._adapter = adapter
    this._path = path
    this._uuid = uuid
    this._descriptors = new Map()
  }

  get descriptors() {
    return this._descriptors
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

  get properties() {
    let properties = 0

    for (const flag of this.flags) {
      if (flag === 'read') properties |= PROPERTY_READ
      else if (flag === 'write-without-response') properties |= PROPERTY_WRITE_WITHOUT_RESPONSE
      else if (flag === 'write') properties |= PROPERTY_WRITE
      else if (flag === 'notify') properties |= PROPERTY_NOTIFY
      else if (flag === 'indicate') properties |= PROPERTY_INDICATE
    }

    return properties
  }

  get mtu() {
    if (this._adapter._destroyed) return undefined
    return binding.charGetMTU(this._adapter._handle, this._path)
  }

  read() {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.charRead(this._adapter._handle, this._path, (err, buffer) => {
        if (err) reject(err)
        else resolve(buffer)
      })
    })
  }

  write(value, opts = {}) {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.charWrite(this._adapter._handle, this._path, value, opts.type, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  startNotify() {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.charStartNotify(this._adapter._handle, this._path, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  stopNotify() {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.charStopNotify(this._adapter._handle, this._path, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Characteristic },
      uuid: this.uuid,
      path: this.path
    }
  }
}

exports.PROPERTY_READ = PROPERTY_READ
exports.PROPERTY_WRITE_WITHOUT_RESPONSE = PROPERTY_WRITE_WITHOUT_RESPONSE
exports.PROPERTY_WRITE = PROPERTY_WRITE
exports.PROPERTY_NOTIFY = PROPERTY_NOTIFY
exports.PROPERTY_INDICATE = PROPERTY_INDICATE
