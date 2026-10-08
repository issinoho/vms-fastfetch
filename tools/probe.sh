#!/usr/bin/env bash
# probe.sh <node> - push the tree, run [.PROBES]PROBE.COM on <node>, and save
# the output to docs/probes-<node>.txt (node details are redacted there by
# hand before committing; the raw log stays in out/).
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: probe.sh <node>}
"$top/tools/push.sh" "$node"
mkdir -p "$top/out"
VMS_TIMEOUT=${VMS_TIMEOUT:-1200} "$top/tools/vms.sh" "$node" dcl \
    'set process/parse_style=extended' '@[.PROBES]PROBE.COM' | tee "$top/out/probes-$node.log"
grep -aq 'PROBE: done' "$top/out/probes-$node.log"
