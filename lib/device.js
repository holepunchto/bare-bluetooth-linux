const binding = require('../binding')
const Service = require('./service')

module.exports = exports = class Device {
  constructor(adapter, path, address) {
    this._adapter = adapter
    this._path = path
    this._address = address
  }

  get path() {
    return this._path
  }

  get address() {
    return this._address
  }

  get name() {
    if (this._adapter._destroyed) return
    return binding.deviceGetName(this._adapter._handle, this._path)
  }

  get rssi() {
    if (this._adapter._destroyed) return
    return binding.deviceGetRSSI(this._adapter._handle, this._path)
  }

  get paired() {
    if (this._adapter._destroyed) return
    return binding.deviceGetPaired(this._adapter._handle, this._path)
  }

  get connected() {
    if (this._adapter._destroyed) return
    return binding.deviceGetConnected(this._adapter._handle, this._path)
  }

  connect(cb) {
    if (this._adapter._destroyed) return
    binding.deviceConnect(this._adapter._handle, this._path, cb)
  }

  disconnect(cb) {
    if (this._adapter._destroyed) return
    binding.deviceDisconnect(this._adapter._handle, this._path, cb)
  }

  pair(cb) {
    if (this._adapter._destroyed) return
    binding.devicePair(this._adapter._handle, this._path, cb)
  }

  discoverServices(uuids, cb) {
    if (typeof uuids === 'function') {
      cb = uuids
      uuids = null
    }

    if (this._adapter._destroyed) return cb(null, [])

    const services = []
    try {
      binding.deviceDiscoverServices(this._adapter._handle, this._path, (path, uuid, primary) => {
        if (!uuids || uuids.includes(uuid)) {
          services.push(new Service(this, path, uuid, primary))
        }
      })
    } catch (err) {
      return cb(err)
    }
    cb(null, services)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Device },
      path: this._path,
      address: this._address,
      name: this.name
    }
  }
}
