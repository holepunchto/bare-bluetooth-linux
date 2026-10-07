export type AgentCapability =
  'DisplayOnly' | 'DisplayYesNo' | 'KeyboardOnly' | 'NoInputNoOutput' | 'KeyboardDisplay'

/**
 * Answers bluetoothd during pairing. Every method gets the D-Bus path of the
 * device. The base class refuses what it was not taught: subclass and resolve
 * to answer, resolve `false` or throw to refuse.
 */
export default class Agent {
  /** bluetoothd needs a PIN code for an authentication */
  requestPinCode(devicePath: string): Promise<string>
  /** bluetoothd needs a passkey for an authentication */
  requestPasskey(devicePath: string): Promise<number>
  /** bluetoothd needs the passkey confirmed, `true` accepts */
  requestConfirmation(devicePath: string, passkey: number): Promise<boolean>
  /** A pairing that shows nothing to confirm or type, `true` accepts */
  requestAuthorization(devicePath: string): Promise<boolean>
  /** A connection or service request, `true` accepts */
  authorizeService(devicePath: string, uuid: string): Promise<boolean>
  /** bluetoothd wants the PIN code shown to the user */
  displayPinCode(devicePath: string, pincode: string): Promise<void>
  /** bluetoothd wants the passkey shown, `entered` counts typed digits */
  displayPasskey(devicePath: string, passkey: number, entered: number): Promise<void>
  /** bluetoothd unregistered the agent */
  release(): Promise<void>
  /** The pending request failed before a reply was returned */
  cancel(): Promise<void>
}
