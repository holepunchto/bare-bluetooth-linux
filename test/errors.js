const test = require('brittle')
const { BluetoothError } = require('..')

test('BluetoothError is exported', (t) => {
  t.is(typeof BluetoothError, 'function')
  t.is(BluetoothError.name, 'BluetoothError')
})

test('each factory carries its code', (t) => {
  for (const code of [
    'SCAN_FAILED',
    'READ_FAILED',
    'WRITE_FAILED',
    'NOTIFY_STATE_FAILED',
    'ADVERTISE_FAILED',
    'SERVICE_ADD_FAILED',
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

test('connection factories carry the peer', (t) => {
  for (const code of ['CONNECTION_FAILED', 'DISCONNECT']) {
    const err = BluetoothError[code]('reason', '00:11:22:33:44:55')

    t.is(err.code, code)
    t.is(err.id, '00:11:22:33:44:55')
  }
})
