import { EventEmitter, EventMap } from 'bare-events'
import Descriptor from './descriptor'

export interface WriteOptions {
  /** `request` waits for the peer's response, `command` does not */
  type?: 'request' | 'command'
}

export interface CharacteristicEventMap extends EventMap {
  /** A notification or indication, while notifying */
  data: [value: ArrayBuffer]
  /** A descriptor bluetoothd just discovered */
  descriptor: [descriptor: Descriptor]
  /** A descriptor bluetoothd already held when the adapter attached */
  descriptorCached: [descriptor: Descriptor]
  descriptorRemoved: [descriptor: Descriptor]
}

export default class Characteristic extends EventEmitter<CharacteristicEventMap> {
  /** D-Bus path of the characteristic */
  readonly path: string
  /** 128-bit characteristic UUID */
  readonly uuid: string
  /** How the value can be used: `read`, `write`, `notify`... */
  readonly flags: string[]
  /** MTU for `read` and `write` */
  readonly mtu: number
  /** Descriptors discovered so far, keyed by D-Bus path */
  readonly descriptors: Map<string, Descriptor>

  /** Reads the value from the peer */
  read(): Promise<ArrayBuffer>
  /** Writes the value to the peer */
  write(value: Uint8Array, opts?: WriteOptions): Promise<void>
  /** Starts a notification session, values arrive on `data` */
  startNotify(): Promise<void>
  /** Stops the session started by `startNotify` */
  stopNotify(): Promise<void>
}
