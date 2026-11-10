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
if [ "${5:-}" != --recover ]; then [ ! -e "$ready" ] && [ ! -L "$ready" ] || exit 2; fi
umask 077
data="$root/data/lge/smartlink/carplay"
request="$data/altscreen-v27-frame-ready"
grant="$data/altscreen-v27-encoder-ready"
attempted=0
owned=0
restore_failed=0
cleaned=0
capture_exports=""
ascii_export_counter=0
expired="$data/altscreen-v27-expired"
token_valid() { [ -f "$1" ] && [ ! -L "$1" ] && [ "$(cat "$1")" = AUTHENTICATED_H264_V27 ]; }
export_reports() {
    for usb in "$root/tmp/USB/PORT1/PART1" "$root/tmp/USB/PORT2/PART1"; do
        [ -d "$usb" ] && [ ! -L "$usb" ] || continue
        [ -f "$usb/ALTSCREEN_TEST_ONCE.txt" ] && [ ! -L "$usb/ALTSCREEN_TEST_ONCE.txt" ] || continue
        for name in boot-status.txt network-status.txt output-diagnostics.txt packet-audit.txt packet-audit-errors.txt; do
            [ -f "$stage/$name" ] && [ ! -L "$stage/$name" ] || continue
            [ -L "$usb/$name" ] || cp "$stage/$name" "$usb/$name" 2>/dev/null || true
        done
        ascii_export_counter=$((ascii_export_counter+1))
        if [ -z "$root" ] && { [ "$owned" = 0 ] || [ $((ascii_export_counter%5)) = 1 ]; };then
            interpreter='';for command in python3 python;do if command -v "$command" >/dev/null 2>&1;then interpreter=$command;break;fi;done
            if [ -n "$interpreter" ];then
                "$interpreter" - "$root" "$usb" >> "$stage/boot-status.txt" 2>&1 <<'PYV27EXPORT'
from __future__ import print_function
import os,sys,stat,json,zlib,base64,hashlib
root,usb=sys.argv[1:3]
if os.geteuid()!=0:raise SystemExit('ABORT: root required')
stage='/lge/app_rw/smartlink/carplay/.moi3-altscreen-map-v27/'
paths=[stage+'boot-status.txt',stage+'packet-audit.txt',stage+'carplay-journal.txt',
 '/data/lge/smartlink/carplay/carplay-handler-v27.log',
 '/lge/app_rw/smartlink/carplay/carplay-handler-v27.log','/tmp/carplay-handler-v27.log',
 '/data/lge/smartlink/carplay/map-v27-stock-info.plist','/data/lge/smartlink/carplay/map-v27-advertised-info.plist']
records=[];total=0
for relative in paths:
 path=root+relative
 try:
  info=os.lstat(path)
  if not stat.S_ISREG(info.st_mode) or info.st_nlink!=1 or info.st_size>2097152:
   records.append({'name':relative,'state':'unsafe-or-oversize'});continue
  # Refuse a symlink in any parent component too.
  if os.path.realpath(path)!=os.path.abspath(path):
   records.append({'name':relative,'state':'unsafe-parent'});continue
  fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
  try:
   current=os.fstat(fd)
   if (info.st_ino,info.st_dev)!=(current.st_ino,current.st_dev):raise ValueError('source changed during open')
   chunks=[];size=0
   while True:
    b=os.read(fd,min(65536,2097153-size))
    if not b:break
    chunks.append(b);size+=len(b)
    if size>2097152:raise ValueError('source exceeds bound')
   data=b''.join(chunks)
   after=os.fstat(fd)
   if current.st_size!=after.st_size or current.st_mtime!=after.st_mtime or len(data)!=after.st_size:raise ValueError('source changed during read')
  finally:os.close(fd)
  total+=len(data)
  if total>8388608:raise ValueError('aggregate bound exceeded')
  record={'name':relative,'state':'read','bytes':len(data),'zeroBytes':data.count(b'\0'),'sha256':hashlib.sha256(data).hexdigest(),'zlibBase64':base64.b64encode(zlib.compress(data,6)).decode('ascii')}
  records.append(record);print('SOURCE',relative,'bytes',record['bytes'],'zeroBytes',record['zeroBytes'],'sha256',record['sha256'])
 except (IOError,OSError) as e:records.append({'name':relative,'state':'unavailable','error':str(e)})
body=json.dumps({'format':'moi3-v27-retained-reports-v1','sourceBytes':total,'unitSideMutations':False,'records':records},sort_keys=True).encode('ascii')
if len(body)>12582912:raise ValueError('encoded bound exceeded')
out=os.path.join(usb,'map-v27-reports.json');pending=out+'.pending'
if os.path.lexists(pending):raise ValueError('existing pending output preserved')
if os.path.lexists(out):
 if os.path.islink(out) or os.path.getsize(out)>12582912:raise ValueError('unsafe previous bundle')
 with open(out,'rb') as previous:
  if json.loads(previous.read(12582913)).get('format')!='moi3-v27-retained-reports-v1':raise ValueError('unknown previous output preserved')
v=os.statvfs(usb)
if v.f_bavail*v.f_frsize<16777216:raise ValueError('USB free space below16MiB')
fd=os.open(pending,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,384)
with os.fdopen(fd,'wb') as f:f.write(body);f.flush();os.fsync(f.fileno())
with open(pending,'rb') as f:check=f.read(12582913)
if hashlib.sha256(check).digest()!=hashlib.sha256(body).digest():raise ValueError('USB readback failed; source preserved')
os.rename(pending,out)
print('BUNDLE SHA256',hashlib.sha256(body).hexdigest(),'bytes',len(body))
print('COLLECTION COMPLETE: compressed ASCII export, fsync and byte readback verified; retained files unchanged')
PYV27EXPORT
            fi
        fi
        # Recover a saved v15 capture as well as this session's v27 capture.
        # Native helper validates records and verifies output by byte readback;
        # no optional BusyBox head/stat flags are required.
        for capture_version in v15 v16 v17 v18 v27; do
            capture="$data/map-$capture_version-video.bin"
            capture_ready="$data/map-$capture_version-video.ready"
            case "$capture_exports" in *"|$capture_version|"*) continue;; esac
            if [ -f "$capture_ready" ] && [ ! -L "$capture_ready" ] && [ -f "$capture" ] && [ ! -L "$capture" ]; then
                capture_exports="$capture_exports|$capture_version|"
                if ! "$stage/file-guard" video-export "$capture" "$capture_ready" "$usb/map-$capture_version-video.bin" >> "$stage/boot-status.txt" 2>&1; then
                    printf 'PRIVATE CAPTURE EXPORT FAILED: version=%s; source preserved where possible; review native error above\n' "$capture_version" >> "$stage/boot-status.txt"
                fi
            fi
        done
        for name in stock-info advertised-info; do
            source="$data/map-v27-$name.plist"
            if [ -f "$source" ] && [ ! -L "$source" ] && [ ! -L "$usb/map-v27-$name.plist" ]; then
                cp "$source" "$usb/map-v27-$name.plist" 2>/dev/null || true
            fi
        done
        for source in "$root/data/lge/smartlink/carplay/carplay-handler-v27.log" "$root/lge/app_rw/smartlink/carplay/carplay-handler-v27.log" "$root/tmp/carplay-handler-v27.log"; do
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
    (set -C; printf 'AUTHENTICATED_H264_V27\n' > "$expired") && chmod 644 "$expired"
}
restore_native() {
    [ "$owned" = 1 ] || return 0
    [ "$restore_failed" = 0 ] || return 1
    if token_valid "$grant"; then rm -f "$grant" || return 1
    elif [ -e "$grant" ] || [ -L "$grant" ]; then
        expire_lease || return 1
        printf '%s\n' 'LEASE WARNING: unknown grant retained for review; output expired' >> "$stage/boot-status.txt"
    fi
    if systemctl is-active --quiet multi-display.service; then
        owned=0;attempted=0
        printf '%s\n' 'NATIVE ALREADY ACTIVE: no duplicate sender started' >> "$stage/boot-status.txt"
        return 0
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
if [ "${5:-}" = --recover ]; then
    # Called independently by systemd, including SIGKILL/runtime timeout.
    # Never start a competing sender: restore_native probes the output port.
    owned=1
    if expire_lease; then
        printf '%s\n' 'POSTSTOP: independently revoked custom output' >> "$stage/boot-status.txt"
        restore_native
        exit $?
    fi
    printf '%s\n' 'POSTSTOP FAILED: revoke could not be written; use normal reboot' >> "$stage/boot-status.txt"
    exit 1
fi
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
