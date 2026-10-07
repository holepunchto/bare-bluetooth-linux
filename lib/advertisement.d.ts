export interface AdvertisementOptions {
  /** Type of advertising packet, `peripheral` by default */
  type?: 'broadcast' | 'peripheral'
  /** Local name for the advertising report, truncated when too long */
  localName?: string
  /** UUIDs for the "Service UUID" field of the advertising data */
  serviceUUIDs?: string[]
  /** Service data elements, keyed by UUID */
  serviceData?: { [uuid: string]: Uint8Array }
}

export default class Advertisement {
  constructor(opts?: AdvertisementOptions)

  readonly type: 'broadcast' | 'peripheral'
  readonly localName: string | undefined
  readonly serviceUUIDs: string[]
  readonly serviceData: { [uuid: string]: Uint8Array }
}
