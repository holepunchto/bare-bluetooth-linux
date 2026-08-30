#!/bin/sh
# Two virtual LE controllers wired together (btvirt) make the server side of
# L2CAP testable end to end without radio hardware. Root required. Prints the
# environment to pass to the test run.
set -e

before=" $(ls /sys/class/bluetooth 2>/dev/null | tr '\n' ' ') "

modprobe hci_vhci
/usr/libexec/bluetooth/btvirt -l2 >/dev/null 2>&1 &
pid=$!

sleep 1

new=""
for hci in $(ls /sys/class/bluetooth); do
  case "$before" in *" $hci "*) ;; *) new="$new $hci" ;; esac
done

set -- $new
if [ $# -ne 2 ]; then
  kill $pid 2>/dev/null
  echo "expected 2 new controllers, got:$new" >&2
  exit 1
fi

echo "BT_VHCI_A=/org/bluez/$1 BT_VHCI_B=/org/bluez/$2"
echo "btvirt pid: $pid (kill it to tear the rig down)"
