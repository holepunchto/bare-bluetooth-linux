const { isCI, poweredAdapter } = require('./test/helpers')

// Every hardware test needs a powered adapter, and would abort on a NotReady
// throw without one, so stop the suite here rather than at the first test
if (!isCI && !poweredAdapter()) {
  console.error('Bluetooth is off on this device, enable it and run again')
  Bare.exitCode = 1
} else {
  require('./test/adapter')
  require('./test/device')
  require('./test/service')
  require('./test/characteristic')
  require('./test/l2cap')
  require('./test/gatt-server')
  require('./test/l2cap-vhci')
}
