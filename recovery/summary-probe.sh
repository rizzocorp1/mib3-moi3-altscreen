#!/bin/sh
# Invoked only by the existing MU0841 summary-screen maintenance hook.
# This script writes a report to USB. It does not install, delete, mount,
# change labels, stop services, or invoke another payload.
umask 077
USB_DIR=''
ROOT=/
if [ -n "${MOI3_TEST_USB:-}" ] && [ -n "${MOI3_TEST_ROOT:-}" ]; then
    USB_DIR=$MOI3_TEST_USB
    ROOT=$MOI3_TEST_ROOT
else
    for candidate in /tmp/USB/PORT1/PART1 /tmp/USB/PORT2/PART1; do
        if [ -f "$candidate/SUMMARY_PROBE_ONLY.txt" ] && [ -f "$candidate/fix_hw_partnumber.sh" ]; then
            [ -z "$USB_DIR" ] || exit 0
            USB_DIR=$candidate
        fi
    done
fi
[ -n "$USB_DIR" ] && [ -d "$USB_DIR" ] || exit 0
[ -f "$USB_DIR/SUMMARY_PROBE_ONLY.txt" ] || exit 0
# Fail closed if someone mixes this with an update or activation stick.
for forbidden in Data Meta ENABLE_ALTSCREEN.txt REVERT.txt PROBE_ONLY.txt; do
    [ ! -e "$USB_DIR/$forbidden" ] && [ ! -L "$USB_DIR/$forbidden" ] || exit 0
done
REPORT="$USB_DIR/summary-probe-report.txt"
[ ! -L "$REPORT" ] || exit 0
{
    echo 'MOI3 SUMMARY HOOK PROBE v1'
    echo 'No update container or activation payload is present.'
    date -u
    echo "Effective UID: $(id -u)"
    uname -m
    echo 'Execution label:'
    [ ! -f "$ROOT/proc/self/attr/current" ] || cat "$ROOT/proc/self/attr/current"
    echo 'Runtime tools:'
    for tool in tar sha256sum readelf chsmack getfattr getfacl stat systemctl; do
        command -v "$tool" 2>/dev/null || echo "MISSING: $tool"
    done
    tar --version 2>&1 | head -2
    echo 'Factory hashes and mode/ownership:'
    for file in lge/app_rw/smartlink/carplay/carplay lge/app_rw/smartlink/carplay/carplay.sh lib64/libc-2.23.so etc/os-release; do
        if [ -f "$ROOT/$file" ] && [ ! -L "$ROOT/$file" ]; then
            sha256sum "$ROOT/$file" 2>&1
            stat -c '%u %g %a %n' "$ROOT/$file" 2>&1
            if command -v getfattr >/dev/null 2>&1; then
                getfattr -n security.SMACK64 "$ROOT/$file" 2>&1
                getfattr -n security.SMACK64EXEC "$ROOT/$file" 2>&1
            fi
        else
            echo "MISSING or SYMLINK: $file"
        fi
    done
    echo 'OS release:'
    [ ! -f "$ROOT/etc/os-release" ] || cat "$ROOT/etc/os-release"
    echo 'Mount inventory:'
    [ ! -f "$ROOT/proc/mounts" ] || cat "$ROOT/proc/mounts"
    echo 'Factory service unit:'
    [ ! -f "$ROOT/lib/systemd/system/carplay.service" ] || cat "$ROOT/lib/systemd/system/carplay.service"
    echo 'RESULT: SUMMARY HOOK REACHED; NO ALTSCREEN INSTALLED'
} > "$REPORT" 2>&1
sync
exit 0
