const EventEmitter = require('bare-events')
const binding = require('../binding')

const L2CAPChannel = require('./channel')
const errors = require('./errors')

module.exports = exports = class Device extends EventEmitter {
  constructor(adapter, path, address) {
    super()
    this._adapter = adapter
    this._path = path
    this._address = address
    this._services = new Map()
  }

  get services() {
    return this._services
  }

  get path() {
    return this._path
  }

  get address() {
    return this._address
  }

  get addressType() {
    if (this._adapter._destroyed) return
    return binding.deviceGetAddressType(this._adapter._handle, this._path)
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

  get servicesResolved() {
    if (this._adapter._destroyed) return
    return binding.deviceGetServicesResolved(this._adapter._handle, this._path)
  }

  get uuids() {
    if (this._adapter._destroyed) return []
    return binding.deviceGetUUIDs(this._adapter._handle, this._path)
  }

  get manufacturerData() {
    if (this._adapter._destroyed) return {}
    const result = {}
    binding.deviceGetManufacturerData(this._adapter._handle, this._path, result)
    return result
  }

  get serviceData() {
    if (this._adapter._destroyed) return {}
    const result = {}
    binding.deviceGetServiceData(this._adapter._handle, this._path, result)
    return result
  }

  connect() {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.deviceConnect(this._adapter._handle, this._path, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  disconnect() {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.deviceDisconnect(this._adapter._handle, this._path, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  openL2CAPChannel(psm) {
    if (this._adapter._destroyed) return
    // Failures fire the native callback synchronously; defer so open is
    // always asynchronous, like the success path
    binding.deviceOpenL2CAPChannel(this._adapter._handle, this._path, psm, (err, channelHandle) => {
      queueMicrotask(() => {
        if (err) {
          this.emit('error', errors.CHANNEL_FAILED(err.message))
          return
        }

        if (!channelHandle) {
          this.emit('error', errors.CHANNEL_FAILED('Channel open failed'))
          return
        }

        this.emit('channelOpen', new L2CAPChannel(channelHandle))
      })
    })
  }

  pair() {
    if (this._adapter._destroyed) return
    return new Promise((resolve, reject) => {
      binding.devicePair(this._adapter._handle, this._path, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Device },
      path: this.path,
      address: this.address,
      name: this.name
    }
  }
}
