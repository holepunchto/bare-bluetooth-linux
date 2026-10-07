export {
  default as Adapter,
  AdapterOptions,
  DiscoveryFilter,
  ChannelOptions,
  AdapterEventMap
} from './lib/adapter'
export { default as Device, AddressType, PreferredBearer, DeviceEventMap } from './lib/device'
export { default as Service, ServiceEventMap } from './lib/service'
export {
  default as Characteristic,
  WriteOptions,
  CharacteristicEventMap
} from './lib/characteristic'
export { default as Descriptor } from './lib/descriptor'
export { default as Advertisement, AdvertisementOptions } from './lib/advertisement'
export { default as Agent, AgentCapability } from './lib/agent'
export { default as L2CAPChannel, L2CAPChannelEventMap } from './lib/channel'
export { default as BluetoothError } from './lib/errors'
export { default as GattApplication, GattApplicationOptions } from './lib/gatt-application'
export { default as GattService, GattServiceOptions } from './lib/gatt-service'
export {
  default as GattCharacteristic,
  GattCharacteristicOptions,
  ReadOptions,
  GattWriteOptions,
  GattCharacteristicEventMap
} from './lib/gatt-characteristic'
export * as constants from './lib/constants'
