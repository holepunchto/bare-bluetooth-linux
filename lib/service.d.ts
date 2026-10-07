import { EventEmitter, EventMap } from 'bare-events'
import Characteristic from './characteristic'

export interface ServiceEventMap extends EventMap {
  /** A characteristic bluetoothd just discovered */
  characteristic: [characteristic: Characteristic]
  /** A characteristic bluetoothd already held when the adapter attached */
  characteristicCached: [characteristic: Characteristic]
  characteristicRemoved: [characteristic: Characteristic]
}

export default class Service extends EventEmitter<ServiceEventMap> {
  /** D-Bus path of the service */
  readonly path: string
  /** 128-bit service UUID */
  readonly uuid: string
  /** Whether this is a primary service, secondary otherwise */
  readonly primary: boolean
  /** Characteristics discovered so far, keyed by D-Bus path */
  readonly characteristics: Map<string, Characteristic>
}
