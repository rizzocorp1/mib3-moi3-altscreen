#!/bin/sh
# Build only against an extracted factory sysroot. Never use a host-linked .so
# as a vehicle payload. This script does not establish ABI or on-car validity.
set -eu
[ "$#" -eq 3 ] || { echo 'usage: build-target.sh HANDLER_SOURCE FACTORY_SYSROOT OUTPUT_SO' >&2; exit 1; }
SOURCE=$1
SYSROOT=$(cd "$2" && pwd -P)
OUT=$3
CC=${MOI3_CC:-aarch64-linux-gnu-gcc}
READELF=${MOI3_READELF:-aarch64-linux-gnu-readelf}
for command in "$CC" "$READELF" sha256sum find; do command -v "$command" >/dev/null; done
library() {
  matches=$(find "$SYSROOT" -type f -name "$1")
  count=$(printf '%s\n' "$matches" | sed '/^$/d' | wc -l)
  [ "$count" -eq 1 ] || { echo "Expected one real $1 in factory sysroot, found $count" >&2; return 1; }
  printf '%s\n' "$matches"
}
# Real filenames are required, rather than symlinks that escape the sysroot.
LIBC=$(library 'libc-*.so')
LIBDL=$(library 'libdl-*.so')
LIBPTHREAD=$(library 'libpthread-*.so')
LOADER=$(library 'ld-*.so')
for lib in "$LIBC" "$LIBDL" "$LIBPTHREAD" "$LOADER"; do
  "$READELF" -h "$lib" | grep -q 'AArch64'
done
sha256sum "$SOURCE" "$LIBC" "$LIBDL" "$LIBPTHREAD" "$LOADER" > "$OUT.inputs.sha256"
"$CC" -shared -fPIC -O2 -Wall -Wextra -fno-stack-protector -nostdlib \
  -Wl,-z,defs -Wl,-rpath-link,"${LIBC%/*}" -Wl,-rpath-link,"${LIBDL%/*}" \
  -Wl,-rpath-link,"${LIBPTHREAD%/*}" -Wl,-rpath-link,"${LOADER%/*}" \
  "$SOURCE" "$LIBDL" "$LIBPTHREAD" "$LIBC" "$LOADER" -o "$OUT"
"$READELF" -h "$OUT" | grep -q 'AArch64'
"$READELF" -d "$OUT" > "$OUT.dynamic.txt"
grep -q 'libdl.so.2' "$OUT.dynamic.txt"
if "$READELF" -r "$OUT" | grep -E 'TLSDESC|TLS_TPREL|TLS_DTP'; then
  echo 'Unexpected TLS relocation; do not deploy' >&2; exit 1
fi
"$READELF" --version-info "$OUT" > "$OUT.versions.txt"
sha256sum "$OUT"
echo 'Target-linked candidate built. Hook ABI, crypto state layout and native-video handoff still require verification.'
