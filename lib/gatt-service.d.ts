import GattCharacteristic from './gatt-characteristic'

export interface GattServiceOptions {
  /** 128-bit service UUID */
  uuid: string
  /** Primary service, `true` by default */
  primary?: boolean
}

export default class GattService {
  constructor(opts: GattServiceOptions)

  readonly uuid: string
  readonly primary: boolean
  readonly characteristics: GattCharacteristic[]

  addCharacteristic(characteristic: GattCharacteristic): void
}
