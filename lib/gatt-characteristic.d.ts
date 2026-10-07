import { EventEmitter, EventMap } from 'bare-events'

export interface ReadOptions {
  /** Offset the peer reads from */
  offset: number
  /** MTU of the link */
  mtu: number
  /** D-Bus path of the reading device */
  device: string
  /** `BR/EDR` or `LE` */
  link: string
}

export interface GattWriteOptions {
  /** Offset the peer writes at */
  offset: number
  /** `command`, `request` or `reliable` */
  type: string
  /** MTU of the link */
  mtu: number
  /** D-Bus path of the writing device */
  device: string
  /** `BR/EDR` or `LE` */
  link: string
  /** The write is part of a prepared, long write */
  prepareAuthorize: boolean
}

export type ReadHandler = (
  options: ReadOptions
) => Uint8Array | ArrayBuffer | Promise<Uint8Array | ArrayBuffer>
export type WriteHandler = (value: Uint8Array, options: GattWriteOptions) => void | Promise<void>

export interface GattCharacteristicOptions {
  /** 128-bit characteristic UUID */
  uuid: string
  /** How the value can be used: `read`, `write`, `notify`, `encrypt-read`... */
  flags?: string[]
  /** Initial value, served when no `read` handler is set */
  value?: Uint8Array
  /** Answers reads, throw with a `code` such as `NotPermitted` to refuse */
  read?: ReadHandler
  /** Sees writes before they are accepted, throw to refuse */
  write?: WriteHandler
}

export interface GattCharacteristicEventMap extends EventMap {
  /** A peer wrote the value */
  write: [value: Uint8Array, options: GattWriteOptions]
  /** A peer started or stopped notifications */
  notifying: [notifying: boolean]
}

export default class GattCharacteristic extends EventEmitter<GattCharacteristicEventMap> {
  constructor(opts: GattCharacteristicOptions)

  readonly uuid: string
  readonly flags: string[]
  /** Whether a peer currently has notifications enabled */
  readonly notifying: boolean
  /** Whether the hierarchy holding it is registered with an adapter */
  readonly registered: boolean
  /** Current value; setting it notifies subscribers, throws when not registered */
  value: Uint8Array
  read: ReadHandler
  write: WriteHandler
}
