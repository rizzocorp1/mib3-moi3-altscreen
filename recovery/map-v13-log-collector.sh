#!/bin/sh
# Run as a separate systemd transient service, never as an ExecStartPre child.
# Arguments: stage directory, ready-file path, iterations (1..300), root prefix.
set -u
stage=$1
ready=$2
iterations=$3
root=${4:-}
case "$iterations" in ''|*[!0-9]*) exit 2;; esac
[ "$iterations" -ge 1 ] && [ "$iterations" -le 300 ] || exit 2
[ -d "$stage" ] && [ ! -L "$stage" ] || exit 2
[ ! -e "$ready" ] && [ ! -L "$ready" ] || exit 2
umask 077
data="$root/data/lge/smartlink/carplay"
request="$data/altscreen-v13-frame-ready"
grant="$data/altscreen-v13-encoder-ready"
attempted=0
owned=0
restore_failed=0
cleaned=0
expired="$data/altscreen-v13-expired"
token_valid() { [ -f "$1" ] && [ ! -L "$1" ] && [ "$(cat "$1")" = AUTHENTICATED_H264_V13 ]; }
export_reports() {
    for usb in "$root/tmp/USB/PORT1/PART1" "$root/tmp/USB/PORT2/PART1"; do
        [ -d "$usb" ] && [ ! -L "$usb" ] || continue
        [ -f "$usb/ALTSCREEN_TEST_ONCE.txt" ] && [ ! -L "$usb/ALTSCREEN_TEST_ONCE.txt" ] || continue
        for name in boot-status.txt network-status.txt output-diagnostics.txt packet-audit.txt packet-audit-errors.txt; do
            [ -f "$stage/$name" ] && [ ! -L "$stage/$name" ] || continue
            [ -L "$usb/$name" ] || cp "$stage/$name" "$usb/$name" 2>/dev/null || true
        done
        for name in stock-info advertised-info; do
            source="$data/map-v13-$name.plist"
            if [ -f "$source" ] && [ ! -L "$source" ] && [ ! -L "$usb/map-v13-$name.plist" ]; then
                cp "$source" "$usb/map-v13-$name.plist" 2>/dev/null || true
            fi
        done
        for source in "$root/data/lge/smartlink/carplay/carplay-handler-v13.log" "$root/lge/app_rw/smartlink/carplay/carplay-handler-v13.log" "$root/tmp/carplay-handler-v13.log"; do
            if [ -f "$source" ] && [ ! -L "$source" ]; then
                [ -L "$usb/altscreen-handler.log" ] || cp "$source" "$usb/altscreen-handler.log" 2>/dev/null || true
            fi
        done
        if command -v journalctl >/dev/null 2>&1 && [ ! -L "$usb/carplay-journal.txt" ]; then
            journalctl -u carplay.service -b -n 150 --no-pager > "$usb/carplay-journal.txt" 2>&1
        fi
    done
}
expire_lease() {
    if [ -e "$expired" ] || [ -L "$expired" ]; then token_valid "$expired"; return; fi
    (set -C; printf 'AUTHENTICATED_H264_V13\n' > "$expired") && chmod 644 "$expired"
}
restore_native() {
    [ "$owned" = 1 ] || return 0
    [ "$restore_failed" = 0 ] || return 1
    if token_valid "$grant"; then rm -f "$grant" || return 1
    elif [ -e "$grant" ] || [ -L "$grant" ]; then
        expire_lease || return 1
        printf '%s\n' 'LEASE WARNING: unknown grant retained for review; output expired' >> "$stage/boot-status.txt"
    fi
    tries=0
    while [ "$tries" -lt 5 ]; do
        if "$stage/packet-audit" --port-free; then
            if systemctl start multi-display.service && systemctl is-active --quiet multi-display.service; then
                owned=0;attempted=0
                printf '%s\n' 'NATIVE RESTORED: custom port free; factory service active' >> "$stage/boot-status.txt"
                return 0
            fi
            restore_failed=1
            printf '%s\n' 'RESTORE FAILED: factory service not active; use normal reboot' >> "$stage/boot-status.txt"
            return 1
        fi
        tries=$((tries+1));sleep 1
    done
    printf '%s\n' 'RESTORE WAIT: custom port still busy; factory sender not started; use normal reboot' >> "$stage/boot-status.txt"
    return 1
}
finish() {
    [ "$cleaned" = 0 ] || return
    cleaned=1
    trap - EXIT INT TERM
    if expire_lease; then
        printf '%s\n' 'LEASE EXPIRED: bounded experiment ended; output permission withdrawn' >> "$stage/boot-status.txt"
        restore_native || true
    else
        printf '%s\n' 'RESTORE FAILED: could not expire output safely; use normal reboot' >> "$stage/boot-status.txt"
    fi
    export_reports;sync
}
trap finish EXIT
trap 'finish; exit 0' INT TERM

# Capture must be live before the experiment can activate. Never change NIC
# membership, promiscuity or firewall policy; keep only bounded packet metadata.
if [ -z "$root" ]; then
    "$stage/packet-audit" eth0.5 "$stage/packet-audit.txt" "$stage/packet-audit-ready" 600 "$grant" > "$stage/packet-audit-errors.txt" 2>&1 &
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
    if [ "$owned" = 1 ] && ! token_valid "$request"; then restore_native || true; fi
    if [ "$owned" = 0 ] && ! token_valid "$request"; then attempted=0; fi
    if [ "$attempted" = 0 ] && [ "$owned" = 0 ] && token_valid "$request" && ! token_valid "$expired"; then
        attempted=1
        if [ -e "$grant" ] || [ -L "$grant" ]; then
            printf '%s\n' 'HANDOFF REFUSED: pre-existing grant retained' >> "$stage/boot-status.txt"
        elif systemctl stop multi-display.service && ! systemctl is-active --quiet multi-display.service; then
            owned=1
            if token_valid "$request" && "$stage/file-guard" clone "$request" "$grant"; then
                printf '%s\n' 'HANDOFF ACTIVE: authenticated video available; encoder grant created' >> "$stage/boot-status.txt"
            else
                printf '%s\n' 'HANDOFF FAILED: request ended or grant creation failed' >> "$stage/boot-status.txt"
                expire_lease || true;restore_native || true
            fi
        else
            printf '%s\n' 'HANDOFF FAILED: native stop not confirmed; test output withheld' >> "$stage/boot-status.txt"
            if "$stage/packet-audit" --port-free; then systemctl start multi-display.service || true; fi
        fi
    fi
    export_reports
    sync
    i=$((i+1))
    sleep 2
done
