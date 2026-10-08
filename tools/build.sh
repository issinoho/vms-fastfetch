#!/usr/bin/env bash
# build.sh <node> [ALL|CLEAN] [KEEP_GOING] - push the tree and run [.VMS]BUILD.COM on <node>.
# The log is printed and saved to out/build-<node>.log.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: build.sh <node> [target] [KEEP_GOING]}
target=${2:-ALL}
keep=${3:-}

"$top/tools/push.sh" "$node"
mkdir -p "$top/out"
job=$top/cache/build-$node.com
cat > "$job" <<'DCL'
$ set noon
$ set process/parse_style=extended
$ purge/nolog [.src...]*.*,[.vms...]*.*
DCL
echo "\$ @[.VMS]BUILD.COM $target $keep" >> "$job"
VMS_TIMEOUT=${VMS_BUILD_TIMEOUT:-1800} "$top/tools/vms.sh" "$node" run "$job" | tee "$top/out/build-$node.log"
grep -aq 'BUILD: done' "$top/out/build-$node.log"
# A link with undefined symbols still writes FASTFETCH.EXE, which then fails
# at run time; with KEEP_GOING, MMS carries on past failed commands and still
# says "BUILD: done".  Treat all of these as failures.
if grep -aE 'USEUNDEF|UNDFSYM|%DCL-[WEF]-|%MMS-[EF]-|%CC-[EF]-|%I?LINK-[EF]-' "$top/out/build-$node.log" >&2; then
    echo "build: errors or undefined symbols in out/build-$node.log" >&2
    exit 1
fi
if grep -aqE '%CC-W-' "$top/out/build-$node.log"; then
    echo "build: compiler warnings in out/build-$node.log" >&2
fi
