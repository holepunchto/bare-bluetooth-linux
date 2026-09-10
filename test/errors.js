const test = require('brittle')
const { BluetoothError } = require('..')

test('BluetoothError is exported', (t) => {
  t.is(typeof BluetoothError, 'function')
  t.is(BluetoothError.name, 'BluetoothError')
})

test('each factory carries its code', (t) => {
  for (const code of [
    'WRITE_FAILED',
    'CHANNEL_FAILED',
    'CHANNEL_PUBLISH_FAILED',
    'AGENT_CANCELED'
  ]) {
    const err = BluetoothError[code]('reason')

    t.ok(err instanceof Error)
    t.is(err.code, code)
    t.is(err.name, 'BluetoothError')
  }
})
