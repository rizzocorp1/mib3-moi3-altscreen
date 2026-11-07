#!/bin/sh
# Run as a separate systemd transient service, never as an ExecStartPre child.
# Arguments: stage directory, ready-file path, iterations (1..150), root prefix.
set -u
stage=$1
ready=$2
iterations=$3
root=${4:-}
case "$iterations" in ''|*[!0-9]*) exit 2;; esac
[ "$iterations" -ge 1 ] && [ "$iterations" -le 150 ] || exit 2
[ -d "$stage" ] && [ ! -L "$stage" ] || exit 2
[ ! -e "$ready" ] && [ ! -L "$ready" ] || exit 2
umask 077
# The caller waits for this handshake before changing CarPlay or its encoder.
(set -C; printf 'READY\n' > "$ready") || exit 2
i=0
while [ "$i" -lt "$iterations" ]; do
    for usb in "$root/tmp/USB/PORT1/PART1" "$root/tmp/USB/PORT2/PART1"; do
        [ -d "$usb" ] && [ ! -L "$usb" ] || continue
        [ -f "$usb/ALTSCREEN_TEST_ONCE.txt" ] && [ ! -L "$usb/ALTSCREEN_TEST_ONCE.txt" ] || continue
        for name in boot-status.txt; do
            [ -f "$stage/$name" ] && [ ! -L "$stage/$name" ] || continue
            [ -L "$usb/$name" ] || cp "$stage/$name" "$usb/$name" 2>/dev/null || true
        done
        for source in "$root/data/lge/smartlink/carplay/carplay-handler-v4.log" "$root/lge/app_rw/smartlink/carplay/carplay-handler-v4.log" "$root/tmp/carplay-handler-v4.log"; do
            if [ -f "$source" ] && [ ! -L "$source" ]; then
                [ -L "$usb/altscreen-handler.log" ] || cp "$source" "$usb/altscreen-handler.log" 2>/dev/null || true
            fi
        done
        if command -v journalctl >/dev/null 2>&1 && [ ! -L "$usb/carplay-journal.txt" ]; then
            journalctl -u carplay.service -b -n 150 --no-pager > "$usb/carplay-journal.txt" 2>&1
        fi
    done
    sync
    i=$((i+1))
    sleep 2
done
