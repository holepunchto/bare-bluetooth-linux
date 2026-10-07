import { EventEmitter, EventMap } from 'bare-events'
import Device from './device'
import Advertisement from './advertisement'
import Agent, { AgentCapability } from './agent'
import GattApplication from './gatt-application'
import L2CAPChannel from './channel'
import BluetoothError from './errors'

export interface AdapterOptions {
  /** D-Bus path of the controller, `/org/bluez/hci0` by default */
  path?: string
}

export interface DiscoveryFilter {
  /** Only report devices advertising one of these service UUIDs */
  uuids?: string[]
  /** Only report devices with an RSSI above this threshold, in dBm */
  rssi?: number
  /** Which transport to scan, `auto` by default */
  transport?: 'auto' | 'bredr' | 'le'
}

export interface ChannelOptions {
  /** PSM to listen on, 0 lets the kernel pick one */
  psm?: number
  /** One of `constants.security`, low by default */
  security?: number
}

export interface AdapterEventMap extends EventMap {
  /** A device bluetoothd just created */
  device: [device: Device]
  /** A device bluetoothd already held when the adapter attached */
  deviceCached: [device: Device]
  deviceRemoved: [device: Device]
  powered: [powered: boolean]
  discovering: [discovering: boolean]
  /** bluetoothd dropped the registered advertisement */
  advertisementReleased: []
  channelPublish: [psm: number]
  /** A peer opened a channel on a published PSM */
  channelOpen: [channel: L2CAPChannel]
  error: [error: BluetoothError]
}

export default class Adapter extends EventEmitter<AdapterEventMap> {
  constructor(opts?: AdapterOptions)

  /** D-Bus path of the controller */
  readonly path: string
  /** Switches the controller on or off, throws when BlueZ refuses */
  powered: boolean
  /** Whether a discovery session is active */
  readonly discovering: boolean
  /** Bluetooth address of the controller, `null` without one at `path` */
  readonly address: string | null
  /** Every device known to bluetoothd, keyed by D-Bus path */
  readonly devices: Map<string, Device>

  /** Sets the discovery filter for this adapter, no filter removes it */
  setDiscoveryFilter(filter?: DiscoveryFilter): Promise<void>
  /** Starts a discovery session: inquiry, scanning and name resolving */
  startDiscovery(): Promise<void>
  /** Stops the session started by `startDiscovery` */
  stopDiscovery(): Promise<void>
  /** Removes the device and its cached information, bonding included */
  removeDevice(device: Device): Promise<void>

  /** Registers an advertisement to be sent over the LE advertising channel */
  registerAdvertisement(advertisement: Advertisement): Promise<void>
  /** Unregisters an advertisement registered with `registerAdvertisement` */
  unregisterAdvertisement(advertisement: Advertisement): Promise<void>

  /** Registers a local GATT service hierarchy, the GATT server role */
  registerApplication(application: GattApplication): Promise<void>
  /** Unregisters the hierarchy registered with `registerApplication` */
  unregisterApplication(application: GattApplication): Promise<void>

  /** Listens for L2CAP connections, `channelPublish` carries the PSM */
  publishL2CAPChannel(opts?: ChannelOptions): void
  /** Stops listening on the PSM */
  unpublishL2CAPChannel(psm: number): void

  /** Registers the pairing agent, `NoInputNoOutput` capability by default */
  registerAgent(agent: Agent, capability?: AgentCapability): Promise<void>
  /** Makes the registered agent the default one for pairing requests */
  requestDefaultAgent(): Promise<void>
  /** Unregisters the agent registered with `registerAgent` */
  unregisterAgent(): Promise<void>

  /** Releases the D-Bus connection, every later call rejects */
  destroy(): void
  [Symbol.dispose](): void
}
