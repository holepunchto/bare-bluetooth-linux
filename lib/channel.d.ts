import { Duplex } from 'bare-stream'

export interface L2CAPChannelEventMap {
  /** The channel is connected and ready for `write` */
  open: []
}

export default class L2CAPChannel extends Duplex {
  /** PSM the channel is connected on */
  readonly psm: number
  /** Bluetooth address of the peer, `null` once closed */
  readonly peer: string | null
  /** Largest chunk `write` accepts */
  readonly mtu: number
}
