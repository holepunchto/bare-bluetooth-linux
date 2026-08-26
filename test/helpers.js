const os = require('bare-os')

exports.isCI = !!os.getEnv('CI')

// Set by test/vhci.sh: D-Bus paths of two virtual controllers wired together
exports.vhci =
  os.getEnv('BT_VHCI_A') && os.getEnv('BT_VHCI_B')
    ? { a: os.getEnv('BT_VHCI_A'), b: os.getEnv('BT_VHCI_B') }
    : null
