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
request="$data/altscreen-v12-frame-ready"
grant="$data/altscreen-v12-encoder-ready"
attempted=0
# Capture must be live before the experiment can activate. Never change NIC
# membership, promiscuity or firewall policy; keep only bounded packet metadata.
if [ -z "$root" ]; then
    "$stage/packet-audit" eth0.5 "$stage/packet-audit.txt" "$stage/packet-audit-ready" 300 "$grant" > "$stage/packet-audit-errors.txt" 2>&1 &
    audit_pid=$!
    attempts=0
    while [ ! -f "$stage/packet-audit-ready" ] && [ "$attempts" -lt 3 ] && kill -0 "$audit_pid" 2>/dev/null; do
        sleep 1
        attempts=$((attempts+1))
    done
    if [ ! -f "$stage/packet-audit-ready" ] || [ -L "$stage/packet-audit-ready" ]; then
        for usb in /tmp/USB/PORT1/PART1 /tmp/USB/PORT2/PART1; do
            if [ -f "$usb/ALTSCREEN_TEST_ONCE.txt" ] && [ ! -L "$usb/packet-audit-errors.txt" ]; then cp "$stage/packet-audit-errors.txt" "$usb/packet-audit-errors.txt" 2>/dev/null || true; fi
        done
        exit 2
    fi
fi
# The caller waits for this handshake before changing CarPlay or its encoder.
(set -C; printf 'READY\n' > "$ready") || exit 2
i=0
while [ "$i" -lt "$iterations" ]; do
    if [ "$i" = 0 ]; then
        {
            printf 'FACTORY TRANSPORT / PRIOR HANDOFF EVIDENCE\n'
            sha256sum "$root/lge/app_ro/graphics/multi-display/multi-screen-server" "$root/lib/systemd/system/multi-display.service" 2>/dev/null || true
            for marker in altscreen-v6-frame-ready altscreen-v6-encoder-ready; do
                prior="$data/$marker"
                if [ -f "$prior" ] && [ ! -L "$prior" ]; then
                    printf 'PRIOR MARKER %s\n' "$marker"
                    stat -c 'mode=%f uid=%u gid=%g size=%s' "$prior" 2>/dev/null || true
                    if command -v getfattr >/dev/null 2>&1; then getfattr -n security.SMACK64 "$prior" 2>/dev/null || true; fi
                fi
            done
        } >> "$stage/output-diagnostics.txt" 2>&1
    fi
    if [ -z "$root" ] && { [ "$i" = 0 ] || [ "$i" = 20 ] || [ "$i" = 45 ]; } && command -v python >/dev/null 2>&1; then
        {
            printf 'DISPLAY SNAPSHOT iteration=%s\n' "$i"
            python - <<'PYDISPLAY'
from __future__ import print_function
import json
try:
    from urllib2 import urlopen
except ImportError:
    from urllib.request import urlopen
try:
    response=urlopen('http://127.0.0.1/',timeout=1)
    listing=json.loads(response.read(262144).decode('utf-8'))
    response.close()
    entries=[entry for entry in listing.get('data',[]) if isinstance(entry,dict) and entry.get('name')=='displaymanagement']
    if len(entries)!=1:
        raise ValueError('display service missing or ambiguous')
    entry=entries[0]
    port=int(entry.get('port',0))
    if port<1 or port>65535 or entry.get('uri')!='/displaymanagement/':
        raise ValueError('invalid local display service endpoint')
    print('DISCOVERED LOCAL DISPLAY PORT',port)
    for name in ('','displays','renderingSurfaces','streamParameters'):
        print('READ ONLY DISPLAYMANAGEMENT',name)
        try:
            response=urlopen('http://127.0.0.1:'+str(port)+'/displaymanagement/'+name+('/' if name else ''),timeout=1)
            print(response.read(262144).decode('utf-8','replace'))
            response.close()
        except Exception as e:
            print(type(e).__name__,str(e))
except Exception as e:
    print(type(e).__name__,str(e))
PYDISPLAY
        } >> "$stage/output-diagnostics.txt" 2>&1
    fi
    # Read-only network evidence; never add, delete or relax a firewall rule.
    if [ "$i" = 5 ] || [ "$i" = 15 ] || [ "$i" = 45 ]; then
        {
            printf 'NETWORK SNAPSHOT iteration=%s\n' "$i"
            if command -v ip >/dev/null 2>&1; then ip -6 addr show dev ncm0; ip -6 route show; fi
            if command -v ss >/dev/null 2>&1; then ss -lnt; fi
            for table in "$root/proc/net/tcp" "$root/proc/net/tcp6"; do
                if [ -f "$table" ]; then printf 'TABLE %s\n' "$table"; cat "$table"; fi
            done
            if command -v ip6tables-save >/dev/null 2>&1; then ip6tables-save; fi
        } >> "$stage/network-status.txt" 2>&1
    fi
    if [ "$attempted" = 0 ] && [ -f "$request" ] && [ ! -L "$request" ] && [ "$(cat "$request")" = AUTHENTICATED_H264_V12 ]; then
        attempted=1
        if systemctl stop multi-display.service && ! systemctl is-active --quiet multi-display.service && "$stage/file-guard" clone "$request" "$grant"; then
            date -u '+HANDOFF TIME %s' >> "$stage/boot-status.txt" 2>/dev/null || true
            stat -c 'GRANT mode=%f uid=%u gid=%g size=%s' "$grant" >> "$stage/output-diagnostics.txt" 2>/dev/null || true
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
        for name in boot-status.txt network-status.txt output-diagnostics.txt packet-audit.txt packet-audit-errors.txt; do
            [ -f "$stage/$name" ] && [ ! -L "$stage/$name" ] || continue
            [ -L "$usb/$name" ] || cp "$stage/$name" "$usb/$name" 2>/dev/null || true
        done
        for name in stock-info advertised-info; do
            source="$data/map-v12-$name.plist"
            if [ -f "$source" ] && [ ! -L "$source" ] && [ ! -L "$usb/map-v12-$name.plist" ]; then
                cp "$source" "$usb/map-v12-$name.plist" 2>/dev/null || true
            fi
        done
        for source in "$root/data/lge/smartlink/carplay/carplay-handler-v12.log" "$root/lge/app_rw/smartlink/carplay/carplay-handler-v12.log" "$root/tmp/carplay-handler-v12.log"; do
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
