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
data="$root/data/lge/smartlink/carplay"
request="$data/altscreen-v6-frame-ready"
grant="$data/altscreen-v6-encoder-ready"
attempted=0
# The caller waits for this handshake before changing CarPlay or its encoder.
(set -C; printf 'READY\n' > "$ready") || exit 2
i=0
while [ "$i" -lt "$iterations" ]; do
    if [ "$attempted" = 0 ] && [ -f "$request" ] && [ ! -L "$request" ] && [ "$(cat "$request")" = AUTHENTICATED_H264_V6 ]; then
        attempted=1
        if systemctl stop multi-display.service && ! systemctl is-active --quiet multi-display.service && "$stage/file-guard" clone "$request" "$grant"; then
            printf '%s\n' 'HANDOFF ACTIVE: authenticated video available; encoder grant created' >> "$stage/boot-status.txt"
        else
            rm -f "$grant"
            if systemctl start multi-display.service; then
                printf '%s\n' 'HANDOFF FAILED: native encoder restarted; test output withheld' >> "$stage/boot-status.txt"
            else
                printf '%s\n' 'HANDOFF FAILED: encoder restart failed; end test with normal reboot' >> "$stage/boot-status.txt"
            fi
        fi
    fi
    for usb in "$root/tmp/USB/PORT1/PART1" "$root/tmp/USB/PORT2/PART1"; do
        [ -d "$usb" ] && [ ! -L "$usb" ] || continue
        [ -f "$usb/ALTSCREEN_TEST_ONCE.txt" ] && [ ! -L "$usb/ALTSCREEN_TEST_ONCE.txt" ] || continue
        for name in boot-status.txt; do
            [ -f "$stage/$name" ] && [ ! -L "$stage/$name" ] || continue
            [ -L "$usb/$name" ] || cp "$stage/$name" "$usb/$name" 2>/dev/null || true
        done
        for name in stock-info advertised-info; do
            source="$data/map-v6-$name.plist"
            if [ -f "$source" ] && [ ! -L "$source" ] && [ ! -L "$usb/map-v6-$name.plist" ]; then
                cp "$source" "$usb/map-v6-$name.plist" 2>/dev/null || true
            fi
        done
        for source in "$root/data/lge/smartlink/carplay/carplay-handler-v6.log" "$root/lge/app_rw/smartlink/carplay/carplay-handler-v6.log" "$root/tmp/carplay-handler-v6.log"; do
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
