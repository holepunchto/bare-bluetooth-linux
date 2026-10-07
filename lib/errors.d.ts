export default class BluetoothError extends Error {
  constructor(msg: string, fn?: Function, code?: string)

  readonly code: string
  readonly name: 'BluetoothError'
  /** Address of the device, on `CONNECTION_FAILED` and `DISCONNECT` */
  id?: string

  static SCAN_FAILED(msg: string): BluetoothError
  static CONNECTION_FAILED(msg: string, id: string): BluetoothError
  static DISCONNECT(msg: string, id: string): BluetoothError
  static READ_FAILED(msg: string): BluetoothError
  static WRITE_FAILED(msg: string): BluetoothError
  static NOTIFY_STATE_FAILED(msg: string): BluetoothError
  static ADVERTISE_FAILED(msg: string): BluetoothError
  static SERVICE_ADD_FAILED(msg: string): BluetoothError
  static CHANNEL_FAILED(msg: string): BluetoothError
  static CHANNEL_PUBLISH_FAILED(msg: string): BluetoothError
  /** The agent threw with this code: bluetoothd sees `Canceled` instead of `Rejected` */
  static AGENT_CANCELED(msg: string): BluetoothError
}
