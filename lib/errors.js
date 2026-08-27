module.exports = class BluetoothError extends Error {
  constructor(msg, fn = BluetoothError, code = fn.name) {
    super(`${code}: ${msg}`)
    this.code = code

    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, fn)
    }
  }

  get name() {
    return 'BluetoothError'
  }

  static WRITE_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.WRITE_FAILED)
  }

  static CHANNEL_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.CHANNEL_FAILED)
  }

  static CHANNEL_PUBLISH_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.CHANNEL_PUBLISH_FAILED)
  }
}
