export type AgentCapability =
  'DisplayOnly' | 'DisplayYesNo' | 'KeyboardOnly' | 'NoInputNoOutput' | 'KeyboardDisplay'

/**
 * Answers bluetoothd during pairing. Every method gets the D-Bus path of the
 * device. The base class refuses what it was not taught: subclass and return,
 * or resolve, to answer; return `false` or throw to refuse.
 */
export default class Agent {
  /** bluetoothd needs a PIN code for an authentication */
  requestPinCode(device: string): string | Promise<string>
  /** bluetoothd needs a passkey for an authentication */
  requestPasskey(device: string): number | Promise<number>
  /** bluetoothd needs the passkey confirmed, `true` accepts */
  requestConfirmation(device: string, passkey: number): boolean | Promise<boolean>
  /** An incoming "just works" pairing, `true` accepts */
  requestAuthorization(device: string): boolean | Promise<boolean>
  /** A connection or service request, `true` accepts */
  authorizeService(device: string, uuid: string): boolean | Promise<boolean>
  /** bluetoothd wants the PIN code shown to the user */
  displayPinCode(device: string, pincode: string): void | Promise<void>
  /** bluetoothd wants the passkey shown, `entered` counts typed digits */
  displayPasskey(device: string, passkey: number, entered: number): void
  /** bluetoothd unregistered the agent */
  release(): void
  /** The pending request failed before a reply was returned */
  cancel(): void
}
