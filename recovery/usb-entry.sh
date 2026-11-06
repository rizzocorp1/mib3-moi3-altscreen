#!/bin/sh
# USB-trigger rehearsal only. Never invokes the upstream installer or payloads.
# Always return 0 to the update framework; inspect the USB report for status.
umask 077
USB_DIR=''
if [ -n "${MOI3_TEST_USB:-}" ] && [ -n "${MOI3_TEST_ROOT:-}" ]; then
  USB_DIR=$MOI3_TEST_USB
  ROOT=$MOI3_TEST_ROOT
else
  ROOT=/
  for candidate in /tmp/USB/PORT1/PART1 /tmp/USB/PORT2/PART1 /media/usb0 /media/usb1 /mnt/usb; do
    if [ -f "$candidate/fix_hw_partnumber.sh" ] && [ -f "$candidate/PROBE_ONLY.txt" ]; then
      if [ -n "$USB_DIR" ]; then echo 'Multiple rehearsal USBs; refusing.' >&2; exit 0; fi
      USB_DIR=$candidate
    fi
  done
fi
[ -n "$USB_DIR" ] && [ -d "$USB_DIR" ] || { echo 'No marked rehearsal USB found.' >&2; exit 0; }
REPORT="$USB_DIR/moi3-usb-probe.txt"
# Refuse symlink report destinations; do not follow them into the unit.
[ ! -L "$REPORT" ] || { echo 'Report path is a symlink; refusing.' >&2; exit 0; }
probe() {
  echo 'MOI3 USB REHEARSAL v1 — no AltScreen installation'
  date -u
  echo "Effective UID: $(id -u)"
  [ -f "$USB_DIR/PROBE_ONLY.txt" ] || { echo 'REFUSED: PROBE_ONLY.txt required'; return 1; }
  for flag in ENABLE_ALTSCREEN.txt REVERT.txt; do
    [ ! -e "$USB_DIR/$flag" ] || { echo "REFUSED: conflicting $flag"; return 1; }
  done
  echo 'Trigger reached. Factory files and services will not be changed.'
  echo 'Tool availability:'
  for tool in tar sha256sum readelf chsmack getfattr getfacl; do
    if command -v "$tool" >/dev/null 2>&1; then command -v "$tool"; else echo "MISSING: $tool"; fi
  done
  tar --version 2>&1 | head -2
  echo 'Factory input inventory (paths and hashes only):'
  for path in lge/app_rw/smartlink/carplay/carplay lge/app_rw/smartlink/carplay/carplay.sh etc/os-release; do
    if [ -f "$ROOT/$path" ] && [ ! -L "$ROOT/$path" ]; then
      if command -v sha256sum >/dev/null 2>&1; then sha256sum "$ROOT/$path"; else echo "PRESENT: $path"; fi
    else echo "MISSING or SYMLINK: $path"; fi
  done
  echo 'USB payload presence (not executed):'
  for path in payload/altscreen_handler.so payload/carplay_launcher factory/carplay recovery/stock-snapshot.sh; do
    if [ -f "$USB_DIR/$path" ]; then echo "PRESENT: $path"; else echo "MISSING: $path"; fi
  done
  echo 'This report proves only that this script was reached, not firmware compatibility or installability.'
}
if probe > "$REPORT" 2>&1; then
  echo 'RESULT: PROBE COMPLETED' >> "$REPORT"
else
  echo 'RESULT: REFUSED OR FAILED — inspect report' >> "$REPORT"
fi
sync
exit 0
