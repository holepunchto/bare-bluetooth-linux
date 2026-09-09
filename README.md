# bare-bluetooth-linux

Linux bluetooth bindings for Bare

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
npx bare test/manual-gatt.js
```

The script walks you through it. It first lists the bonds this machine still holds and offers to forget one, then publishes a Heart Rate service and prints the five steps to follow on the phone. Each step ticks off what it observes - the agent answering a pairing, the subscription, the read, the write, the device path carried in the request options - and it prints its own summary once all six are seen:

```
  [ok] central connected  -- 6F:15:84:82:41:E7   (1/6)
  [ok] agent answered a pairing request  -- authorization   (2/6)
  ...
all 6 checks observed, the gatt server works end to end
```

You need a BLE explorer on the phone: nRF Connect or LightBlue, both free.

Clear the bond on **both** sides before a run. A bond forgotten on one side only leaves the other holding keys the peer no longer has, and every later pairing fails with no useful error. Use the script to remove the bond on Linux side. Do it manually on the phone side. Phones also cache the GATT service list per peripheral and never re-read it, so a stale bond will have you staring at a service tree from a previous session, wondering why your changes do nothing.
