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

  static SCAN_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.SCAN_FAILED)
  }

  static CONNECTION_FAILED(msg, id) {
    const err = new BluetoothError(msg, BluetoothError.CONNECTION_FAILED)
    err.id = id
    return err
  }

  static DISCONNECT(msg, id) {
    const err = new BluetoothError(msg, BluetoothError.DISCONNECT)
    err.id = id
    return err
  }

  static READ_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.READ_FAILED)
  }

  static WRITE_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.WRITE_FAILED)
  }

  static NOTIFY_STATE_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.NOTIFY_STATE_FAILED)
  }

  static ADVERTISE_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.ADVERTISE_FAILED)
  }

  static SERVICE_ADD_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.SERVICE_ADD_FAILED)
  }

  static CHANNEL_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.CHANNEL_FAILED)
  }

  static CHANNEL_PUBLISH_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.CHANNEL_PUBLISH_FAILED)
  }

  static AGENT_CANCELED(msg) {
    return new BluetoothError(msg, BluetoothError.AGENT_CANCELED)
  }
}
