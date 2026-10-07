import { EventEmitter, EventMap } from 'bare-events'

export default class Descriptor extends EventEmitter<EventMap> {
  /** D-Bus path of the descriptor */
  readonly path: string
  /** 128-bit descriptor UUID */
  readonly uuid: string
  /** How the value can be used: `read`, `write`... */
  readonly flags: string[]

  /** Reads the value from the peer */
  read(): Promise<ArrayBuffer>
  /** Writes the value to the peer */
  write(value: Uint8Array): Promise<void>
}
