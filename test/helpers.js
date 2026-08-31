const os = require('bare-os')
const { Adapter } = require('..')

exports.isCI = !!os.getEnv('CI')

// Set by test/vhci.sh: D-Bus paths of two virtual controllers wired together
exports.vhci =
  os.getEnv('BT_VHCI_A') && os.getEnv('BT_VHCI_B')
    ? { a: os.getEnv('BT_VHCI_A'), b: os.getEnv('BT_VHCI_B') }
    : null

exports.poweredAdapter = function poweredAdapter() {
  const adapter = new Adapter()
  // Powers the controller on, unless Bluetooth is off on the device itself
  adapter.powered = true
  const powered = adapter.powered
  adapter.destroy()
  return powered
}
