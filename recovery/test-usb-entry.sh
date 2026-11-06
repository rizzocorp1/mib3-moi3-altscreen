#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
case "$TMP" in /tmp/*) ;; *) exit 1;; esac
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/usb" "$TMP/unit/lge/app_rw/smartlink/carplay" "$TMP/unit/etc"
printf 'factory\n' > "$TMP/unit/lge/app_rw/smartlink/carplay/carplay.sh"
before=$(sha256sum "$TMP/unit/lge/app_rw/smartlink/carplay/carplay.sh")
touch "$TMP/usb/PROBE_ONLY.txt"
MOI3_TEST_USB="$TMP/usb" MOI3_TEST_ROOT="$TMP/unit" sh "$HERE/usb-entry.sh"
grep -q 'RESULT: PROBE COMPLETED' "$TMP/usb/moi3-usb-probe.txt"
[ "$before" = "$(sha256sum "$TMP/unit/lge/app_rw/smartlink/carplay/carplay.sh")" ]
touch "$TMP/usb/ENABLE_ALTSCREEN.txt"
MOI3_TEST_USB="$TMP/usb" MOI3_TEST_ROOT="$TMP/unit" sh "$HERE/usb-entry.sh"
grep -q 'REFUSED: conflicting ENABLE_ALTSCREEN.txt' "$TMP/usb/moi3-usb-probe.txt"
grep -q 'RESULT: REFUSED' "$TMP/usb/moi3-usb-probe.txt"
[ "$before" = "$(sha256sum "$TMP/unit/lge/app_rw/smartlink/carplay/carplay.sh")" ]
echo 'PASS: USB rehearsal reports execution, rejects install flags, preserves factory file.'
