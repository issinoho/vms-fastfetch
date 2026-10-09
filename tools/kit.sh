#!/usr/bin/env bash
# kit.sh <node> - build the PCSI kit on <node> and fetch it to out/kits/.
# Builds first (via build.sh) so the kit always matches the pushed tree.
# Checks that vms/kit/kit.env's KIT_VERSION matches FF_PROGRAM_VERSION.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: kit.sh <node>}
read -r _ ARCH _ _ _ WORKDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")

kitver=$(sed -n 's/^KIT_VERSION=//p' "$top/vms/kit/kit.env")
progver=$(sed -n 's/^#define FF_PROGRAM_VERSION "\(.*\)"/\1/p' "$top/src/fastfetch.h")
[ "$kitver" = "$progver" ] ||
    { echo "kit: kit.env KIT_VERSION $kitver != FF_PROGRAM_VERSION $progver" >&2; exit 1; }

mkdir -p "$top/out"
# build.sh writes out/build-<node>.log itself; don't redirect into it.
bash "$top/tools/build.sh" "$node" > /dev/null 2>&1 ||
    { tail -20 "$top/out/build-$node.log"; echo "kit: build failed" >&2; exit 1; }

job=$top/cache/kit-$node.com
printf '$ set noon\n$ @%s.VMS.KIT]MAKE_KIT.COM\n' "${WORKDIR%]}" > "$job"
VMS_TIMEOUT=1800 bash "$top/tools/vms.sh" "$node" run "$job" | tee "$top/out/kit-$node.log"
kit=$(sed -n 's/^MAKE_KIT: kit .*\]\([^;]*\);.*/\1/p' "$top/out/kit-$node.log" | head -1)
[ -n "$kit" ] || { echo "kit: no kit produced (see out/kit-$node.log)" >&2; exit 1; }
mkdir -p "$top/out/kits"
bash "$top/tools/vms.sh" "$node" get "KIT_$ARCH/$kit" "$top/out/kits/$kit"
ls -la "$top/out/kits/$kit"
