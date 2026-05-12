const test = require('brittle')
const { Device } = require('..')

test('device has expected properties', (t) => {
  t.is(typeof Device, 'function')
  t.is(Device.name, 'Device')
})
