// Guided manual check of the GATT server against a real central (a phone).
// Everything here needs a peer that can pair and stay connected, which no
// virtual controller on this machine can do. Run it, follow the steps, watch
// the checklist fill in.
//
//   bare test/manual-gatt.js

const stdio = require('bare-stdio')
const {
  Adapter,
  Advertisement,
  Agent,
  GattApplication,
  GattService,
  GattCharacteristic
} = require('..')

const SERVICE = '0000180d-0000-1000-8000-00805f9b34fb'
const CHARACTERISTIC = '00002a37-0000-1000-8000-00805f9b34fb'

const checks = {
  'agent answered a pairing request': false,
  'central connected': false,
  'central subscribed (StartNotify)': false,
  'central read (ReadValue)': false,
  'central wrote (WriteValue)': false,
  'request options carried a device path': false
}

const total = Object.keys(checks).length

function pass(name, detail) {
  if (checks[name]) return
  checks[name] = true

  const done = Object.keys(checks).filter((check) => checks[check]).length

  console.log(
    '  [ok] ' + name + (detail ? '  -- ' + detail : '') + '   (' + done + '/' + total + ')'
  )

  if (done === total) report()
}

class JustWorks extends Agent {
  requestConfirmation(device, passkey) {
    pass('agent answered a pairing request', 'confirmation, passkey ' + passkey)
  }

  requestAuthorization(device) {
    pass('agent answered a pairing request', 'authorization')
  }

  authorizeService(device, uuid) {
    pass('agent answered a pairing request', 'service ' + uuid)
  }
}

const adapter = new Adapter()
adapter.powered = true

const characteristic = new GattCharacteristic({
  uuid: CHARACTERISTIC,
  flags: ['read', 'write', 'notify'],
  value: new Uint8Array([0, 60])
})

let beats = 60

characteristic.read = (options) => {
  pass('central read (ReadValue)', 'offset ' + options.offset)
  if (options.device) pass('request options carried a device path', options.device)
  return new Uint8Array([0, beats])
}

characteristic.on('write', (value, options) => {
  pass('central wrote (WriteValue)', '[' + Array.from(value).join(', ') + ']')
  if (options.device) pass('request options carried a device path', options.device)
})

let pushing = null

characteristic.on('notifying', (notifying) => {
  if (notifying) {
    pass('central subscribed (StartNotify)', 'pushing a new value every second')

    pushing = setInterval(() => {
      beats = beats === 90 ? 60 : beats + 1
      characteristic.value = new Uint8Array([0, beats])
    }, 1000)
  } else {
    clearInterval(pushing)
    pushing = null
    console.log('  [--] central unsubscribed (StopNotify)')
  }
})

// BlueZ hands over the devices it already knows at construction, so a peer that
// connected before this ran fires no 'device' event: watch both
function watch(device) {
  if (device.connected) pass('central connected', device.address)

  device.on('connected', (connected) => {
    if (connected) pass('central connected', device.address)
  })
}

function report() {
  const left = Object.keys(checks).filter((name) => !checks[name])

  if (left.length === 0) {
    console.log('\nall ' + total + ' checks observed, the gatt server works end to end')
    return
  }

  console.log('\n' + (total - left.length) + '/' + total + ' checks observed, still missing:')
  for (const name of left) console.log('  [  ] ' + name)
}

// A bond lives on both sides. Forgetting it on the phone alone leaves BlueZ
// holding keys the phone no longer has, and every later pairing fails
async function forgetBond() {
  const bonded = [...adapter.devices.values()].filter((device) => device.paired)

  if (bonded.length === 0) return

  console.log('bonds this machine still holds:')
  bonded.forEach((device, i) => {
    console.log('  ' + (i + 1) + '  ' + device.address + '  ' + (device.name || 'unnamed'))
  })

  console.log('\nwhich one is the phone? type its number to forget it here,')
  console.log('or press enter to leave them all alone')

  const answer = await new Promise((resolve) => {
    stdio.in.once('data', (data) => resolve(data.toString().trim()))
  })

  const picked = bonded[Number(answer) - 1]

  if (!picked) {
    console.log('left alone\n')
    return
  }

  const name = picked.name || 'unnamed'
  const address = picked.address

  await adapter.removeDevice(picked)
  console.log('forgot ' + address + ' (' + name + ')\n')
}

async function main() {
  // BlueZ hands over the devices it already knows on the next loop turn, not
  // during the constructor, so there is nothing to read yet
  await new Promise((resolve) => setTimeout(resolve, 1000))

  await forgetBond()

  const app = new GattApplication({ path: '/com/bare/manual' })
  const service = new GattService({ uuid: SERVICE })

  // A second characteristic and a second service on purpose: the checks all hit
  // the first characteristic, whose registration used to dangle once more were added
  service.addCharacteristic(characteristic)
  service.addCharacteristic(
    new GattCharacteristic({
      uuid: '00002a39-0000-1000-8000-00805f9b34fb',
      flags: ['write']
    })
  )

  const battery = new GattService({ uuid: '0000180f-0000-1000-8000-00805f9b34fb' })
  battery.addCharacteristic(
    new GattCharacteristic({
      uuid: '00002a19-0000-1000-8000-00805f9b34fb',
      flags: ['read'],
      value: new Uint8Array([100])
    })
  )

  app.addService(service)
  app.addService(battery)

  await adapter.registerAgent(new JustWorks(), 'NoInputNoOutput')
  await adapter.requestDefaultAgent()
  await adapter.registerApplication(app)
  await adapter.registerAdvertisement(
    new Advertisement({
      type: 'peripheral',
      localName: 'bare-check',
      serviceUUIDs: [SERVICE]
    })
  )

  console.log('advertising as "bare-check", Heart Rate service\n')
  console.log('Before you start, forget this machine in the phone bluetooth')
  console.log('settings, and clear its bond here too (see above), or the')
  console.log('pairing fails and the phone shows a stale service list.\n')
  console.log('Then, in nRF Connect or LightBlue:')
  console.log('  1. scan and CONNECT to "bare-check"     -> connected, agent answers')
  console.log('  2. open Heart Rate > Heart Rate Measurement')
  console.log('  3. tap notify, the value counts up      -> subscribed')
  console.log('  4. tap read                             -> read, request options')
  console.log('  5. tap write, send any bytes            -> written\n')
  console.log('the summary prints itself once all 6 are seen, ctrl-c to stop\n')

  for (const device of adapter.devices.values()) watch(device)

  adapter.on('device', watch)
}

Bare.on('exit', report)

main().catch((err) => {
  console.error('failed:', err.message)
  Bare.exitCode = 1
})
