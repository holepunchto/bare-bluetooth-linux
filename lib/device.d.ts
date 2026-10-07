import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import L2CAPChannel from './channel'
import BluetoothError from './errors'

export type AddressType = 'public' | 'random'
export type PreferredBearer = 'last-used' | 'bredr' | 'le' | 'last-seen'

export interface DeviceEventMap extends EventMap {
  connected: [connected: boolean]
  paired: [paired: boolean]
  servicesResolved: [resolved: boolean]
  rssi: [rssi: number]
  name: [name: string]
  /** A service bluetoothd just discovered */
  service: [service: Service]
  /** A service bluetoothd already held when the adapter attached */
  serviceCached: [service: Service]
  serviceRemoved: [service: Service]
  /** The channel requested with `openL2CAPChannel` is connected */
  channelOpen: [channel: L2CAPChannel]
  error: [error: BluetoothError]
}

export default class Device extends EventEmitter<DeviceEventMap> {
  /** D-Bus path of the device */
  readonly path: string
  /** Bluetooth address of the remote device */
  readonly address: string
  /** `public` for dual-mode and BR/EDR devices, either for LE only ones */
  readonly addressType: AddressType
  /** Remote name, once a scan response or a connection carried it */
  readonly name: string | undefined
  /** Signal strength of the last advertisement, in dBm */
  readonly rssi: number | undefined
  /** Whether the devices exchanged keys for an encrypted connection */
  readonly paired: boolean
  readonly connected: boolean
  /** Whether the GATT tree in `services` is complete */
  readonly servicesResolved: boolean
  /** 128-bit UUIDs of the remote services */
  readonly uuids: string[]
  /** Manufacturer specific advertisement data, keyed by manufacturer id */
  readonly manufacturerData: { [id: number]: Uint8Array }
  /** Service advertisement data, keyed by UUID */
  readonly serviceData: { [uuid: string]: Uint8Array }
  /** Services discovered so far, keyed by D-Bus path */
  readonly services: Map<string, Service>
  /** Bearer to try first when connecting, dual-mode devices only */
  preferredBearer: PreferredBearer | undefined

  /** Connects every profile the device supports and flags auto-connectable */
  connect(): Promise<void>
  /** Disconnects every profile, then the ACL link */
  disconnect(): Promise<void>
  /** Connects, pairs, then discovers services */
  pair(): Promise<void>
  /** Opens an L2CAP channel on the PSM, delivered on `channelOpen` */
  openL2CAPChannel(psm: number, opts?: { security?: number }): void
}
