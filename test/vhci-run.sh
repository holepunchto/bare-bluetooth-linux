#!/bin/sh
# One-shot vhci test run: rig up as root, tests as you, rig down.
# Usage: sh test/vhci-run.sh [test file...] (default: test/l2cap-vhci.js)
#
# Run this as your normal user, not under sudo: only the rig needs root, and
# dropping back down with `sudo -u` would lose the PATH that version managers
# (mise, nvm, fnm) put node and npx on.
set -e

[ "$(id -u)" -ne 0 ] || { echo "run as your normal user; the script sudoes for the rig only" >&2; exit 1; }

dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
out=$(sudo sh "$dir/vhci.sh")
vars=$(echo "$out" | sed -n 1p)
pid=$(echo "$out" | sed -n 's/^btvirt pid: \([0-9]*\).*/\1/p')
trap 'sudo kill "$pid" 2>/dev/null' EXIT INT TERM

cd "$dir/.."
[ $# -gt 0 ] || set -- test/l2cap-vhci.js
env $vars npx brittle-bare "$@"
