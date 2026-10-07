import GattService from './gatt-service'

export interface GattApplicationOptions {
  /** D-Bus path to export the hierarchy at */
  path?: string
}

export default class GattApplication {
  constructor(opts?: GattApplicationOptions)

  readonly path: string
  readonly services: GattService[]

  addService(service: GattService): void
}
