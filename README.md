> [!IMPORTANT]
> This module is experimental. The API is subject to change and may break at any time.

# bare-bluetooth-linux

BlueZ bindings for Bare. Provides central and peripheral roles, GATT services and characteristics, and L2CAP channels on Linux.

```
npm i bare-bluetooth-linux
```

## Usage

Central, scanning for a Heart Rate monitor and reading its measurement:

```js
const { Adapter } = require('bare-bluetooth-linux')

const adapter = new Adapter()

adapter.on('device', async (device) => {
  if (!device.uuids.includes('0000180d-0000-1000-8000-00805f9b34fb')) return

  await adapter.stopDiscovery()
  await device.connect()

  device.on('servicesResolved', (resolved) => {
    if (!resolved) return

    for (const service of device.services.values()) {
      for (const characteristic of service.characteristics.values()) {
        if (characteristic.flags.includes('notify')) {
          characteristic.on('data', (value) => console.log(new Uint8Array(value)))
          characteristic.startNotify()
        }
      }
    }
  })
})

adapter.setDiscoveryFilter({ transport: 'le' }).then(() => adapter.startDiscovery())
```

Peripheral, publishing a Heart Rate service and notifying a new value every second:

```js
const {
  Adapter,
  Advertisement,
  Agent,
  GattApplication,
  GattService,
  GattCharacteristic
} = require('bare-bluetooth-linux')

const SERVICE = '0000180d-0000-1000-8000-00805f9b34fb'

const characteristic = new GattCharacteristic({
  uuid: '00002a37-0000-1000-8000-00805f9b34fb',
  flags: ['read', 'notify'],
  value: new Uint8Array([0, 60])
})

const service = new GattService({ uuid: SERVICE })
service.addCharacteristic(characteristic)

const app = new GattApplication({ path: '/com/example/heart' })
app.addService(service)

class AcceptAll extends Agent {
  requestAuthorization() {
    return Promise.resolve(true)
  }
}

const adapter = new Adapter()

async function main() {
  await adapter.registerAgent(new AcceptAll())
  await adapter.requestDefaultAgent()
  await adapter.registerApplication(app)
  await adapter.registerAdvertisement(
    new Advertisement({ localName: 'bare', serviceUUIDs: [SERVICE] })
  )

  setInterval(() => {
    characteristic.value = new Uint8Array([0, 60 + Math.round(Math.random() * 10)])
  }, 1000)
}

main()
```

## API

See [`index.d.ts`](./index.d.ts) and the declarations next to each module in [`lib/`](./lib).

## Testing

```
npm test
```

Runs the suite against the machine's own controller. Bluetooth must be on: the suite stops immediately with a message rather than failing test by test. Tests needing real hardware skip themselves under `CI`.

### L2CAP over virtual controllers

`test/l2cap-vhci.js` exercises both ends of an L2CAP channel without radio hardware, using two `btvirt` controllers wired together. It needs root, so it runs through a helper:

```
sh test/vhci-run.sh test/l2cap-vhci.js
```

`btvirt` comes from the BlueZ sources and most distributions do not ship it. Debian and Ubuntu have it in `bluez-test-tools`; on Arch you have to build it yourself. Set `BTVIRT` if it lands somewhere unusual.

If a run is interrupted, `btvirt` can survive and leave its controllers behind. They pile up across runs and confuse BlueZ, so check with `bluetoothctl list` and kill any leftover process before blaming the tests.

### The GATT server needs a real central

For the GATT server, the pairing agent and the request options are checked by hand, against a phone:

```
bare test/manual-gatt.js
```

The script walks you through it. It first lists the bonds this machine still holds and offers to forget one, then publishes a Heart Rate service and prints the five steps to follow on the phone. Each step ticks off what it observes - the agent answering a pairing, the subscription, the read, the write, the device path carried in the request options - and it prints its own summary once all seven are seen:

```
  [ok] central connected  -- 6F:15:84:82:41:E7   (1/7)
  [ok] agent answered a pairing request  -- authorization   (2/7)
  ...
all 7 checks observed, the gatt server works end to end
```

You need a BLE explorer on the phone: nRF Connect or LightBlue, both free.

Clear the bond on **both** sides before a run. A bond forgotten on one side only leaves the other holding keys the peer no longer has, and every later pairing fails with no useful error. Use the script to remove the bond on Linux side. Do it manually on the phone side. Phones also cache the GATT service list per peripheral and never re-read it, so a stale bond will have you staring at a service tree from a previous session, wondering why your changes do nothing.

## License

Apache-2.0
