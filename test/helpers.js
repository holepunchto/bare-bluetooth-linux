const os = require('bare-os')
const { Adapter } = require('..')

exports.isCI = !!os.getEnv('CI')

// Set by test/vhci.sh: D-Bus paths of two virtual controllers wired together
exports.vhci =
  os.getEnv('BT_VHCI_A') && os.getEnv('BT_VHCI_B')
    ? { a: os.getEnv('BT_VHCI_A'), b: os.getEnv('BT_VHCI_B') }
    : null

// For tests that only need some device, whether bluetoothd already held it or
// just heard it
exports.onAnyDevice = function onAnyDevice(adapter, fn) {
  adapter.on('device', fn)
  adapter.on('deviceCached', fn)
}

// The first device heard, cached or not
exports.findDevice = async function findDevice(adapter) {
  const found = new Promise((resolve) => exports.onAnyDevice(adapter, resolve))
  await adapter.startDiscovery()
  const device = await found
  await adapter.stopDiscovery()
  return device
}

exports.poweredAdapter = function poweredAdapter() {
  const adapter = new Adapter()
  let powered = false

  try {
    // Powers the controller on, unless Bluetooth is off on the device itself
    adapter.powered = true
    powered = adapter.powered
  } catch {
    // BlueZ refused outright, so it stays off
  }

  adapter.destroy()
  return powered
}
