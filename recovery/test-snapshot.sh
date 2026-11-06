#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
case "$TMP" in /tmp/*) ;; *) echo 'Unexpected temp path' >&2; exit 1;; esac
trap 'rm -rf "$TMP"' EXIT
ROOT="$TMP/unit"
mkdir -p "$ROOT/etc" "$ROOT/lge/app_rw/smartlink/carplay" "$ROOT/lge/app_rw/lib64" "$ROOT/data/lge/smartlink"
printf 'test-unit\n' > "$ROOT/etc/machine-id"
printf 'VERSION=test\n' > "$ROOT/etc/os-release"
printf 'factory-script\n' > "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh"
printf 'stock-binary\n' > "$ROOT/lge/app_rw/smartlink/carplay/carplay"
printf 'pairing\n' > "$ROOT/data/lge/smartlink/pairing"
export MOI3_TEST_ROOT="$ROOT"
case "$(uname -s)" in MINGW*|MSYS*) export MOI3_TEST_CONTENT_ONLY=YES;; esac
if [ "${MOI3_TEST_CONTENT_ONLY:-}" != YES ]; then
  chmod 750 "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh"
  python3 -c 'import os,sys; os.setxattr(sys.argv[1], "user.moi3-test", b"stock-label")' "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh"
fi
sh "$HERE/stock-snapshot.sh" backup "$ROOT" "$TMP/baseline"
if sh "$HERE/stock-snapshot.sh" backup "$ROOT" "$TMP/baseline"; then exit 1; fi
mkdir "$TMP/incomplete"
if sh "$HERE/stock-snapshot.sh" verify "$ROOT" "$TMP/incomplete"; then exit 1; fi
printf 'different-unit\n' > "$ROOT/etc/machine-id"
if sh "$HERE/stock-snapshot.sh" verify "$ROOT" "$TMP/baseline"; then exit 1; fi
printf 'test-unit\n' > "$ROOT/etc/machine-id"
printf 'changed\n' > "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh"
printf 'new-hook\n' > "$ROOT/lge/app_rw/lib64/altscreen_handler.so"
printf 'added\n' > "$ROOT/lge/app_rw/smartlink/carplay/DO_TEST_ONCE"
if sh "$HERE/stock-snapshot.sh" restore "$ROOT" "$TMP/baseline"; then exit 1; fi
MOI3_SERVICES_STOPPED=YES MOI3_FIRMWARE_MATCH_CONFIRMED=YES sh "$HERE/stock-snapshot.sh" restore "$ROOT" "$TMP/baseline"
grep -q '^factory-script$' "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh"
if [ "${MOI3_TEST_CONTENT_ONLY:-}" != YES ]; then
  [ "$(stat -c %a "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh")" = 750 ]
  python3 -c 'import os,sys; assert os.getxattr(sys.argv[1], "user.moi3-test") == b"stock-label"' "$ROOT/lge/app_rw/smartlink/carplay/carplay.sh"
fi
[ ! -e "$ROOT/lge/app_rw/lib64/altscreen_handler.so" ]
[ ! -e "$ROOT/lge/app_rw/smartlink/carplay/DO_TEST_ONCE" ]
grep -q '^changed$' "$ROOT/lge/app_rw/smartlink/carplay.moi3-rollback/carplay.sh"
if MOI3_SERVICES_STOPPED=YES MOI3_FIRMWARE_MATCH_CONFIRMED=YES sh "$HERE/stock-snapshot.sh" restore "$ROOT" "$TMP/baseline"; then exit 1; fi
printf 'corruption' >> "$TMP/baseline/stock.tar"
if sh "$HERE/stock-snapshot.sh" verify "$ROOT" "$TMP/baseline"; then exit 1; fi
echo "PASS: originals restored; additions removed; quarantine retained; invalid operations rejected. Content-only mode: ${MOI3_TEST_CONTENT_ONLY:-NO}. Real SMACK/target validation remains required."
