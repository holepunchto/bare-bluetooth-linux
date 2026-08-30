#!/bin/sh
# One-shot vhci test run: rig up, tests as the calling user, rig down.
# Usage: sudo sh test/vhci-run.sh [test file...] (default: test/l2cap-vhci.js)
set -e

[ "$(id -u)" -eq 0 ] || { echo "run with sudo" >&2; exit 1; }

dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
out=$(sh "$dir/vhci.sh")
vars=$(echo "$out" | sed -n 1p)
pid=$(echo "$out" | sed -n 's/^btvirt pid: \([0-9]*\).*/\1/p')
trap 'kill "$pid" 2>/dev/null' EXIT INT TERM

cd "$dir/.."
[ $# -gt 0 ] || set -- test/l2cap-vhci.js
sudo -u "${SUDO_USER:-root}" env $vars npx brittle-bare "$@"
