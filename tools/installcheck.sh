#!/usr/bin/env bash
# installcheck.sh <node> - install the node's kit, verify it, run the smoke test
# against the installed image, and remove it again.  This changes the system
# while it runs (PCSI database, SYS$COMMON:[FASTFETCH], FASTFETCH$ROOT,
# FASTFETCH$PRESETS); ask before running it.  Run kit.sh first.
# Output: out/install-<node>.txt.
set -euo pipefail
top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: installcheck.sh <node>}
bash "$top/tools/vms.sh" "$node" put "$top/tools/vms_installcheck.com" >/dev/null
job=$top/cache/installcheck-$node.com
printf '$ set noon\n$ @VMS_INSTALLCHECK.COM\n' > "$job"
mkdir -p "$top/out"
VMS_TIMEOUT=1800 bash "$top/tools/vms.sh" "$node" run "$job" > "$top/out/install-$node.txt" 2>&1
# Occasionally (seen on IA64) the session returns an empty log without
# having run anything; the system is then untouched.  Say so: retry.
if [ ! -s "$top/out/install-$node.txt" ]; then
    echo "installcheck: empty log - the procedure did not run; check the node is clean and retry" >&2
    exit 1
fi
grep -aE 'install status|Installed|startup procedure|vms-fastfetch [0-9]|SMOKE:|SUCREMOVE|remove status|after removal|items found' \
    "$top/out/install-$node.txt"
# The installed program must be this tree's version: a failed kit.sh once
# left the previous kit in [.KIT_<arch>], and that was installed instead.
kitver=$(sed -n 's/^KIT_VERSION=//p' "$top/vms/kit/kit.env")
grep -q "vms-fastfetch $kitver " "$top/out/install-$node.txt" ||
    { echo "installcheck: the installed kit is not version $kitver; rerun tools/kit.sh $node" >&2; exit 1; }
grep -q 'SMOKE: [0-9]* passed, 0 failed' "$top/out/install-$node.txt" &&
    grep -q 'FASTFETCH\$ROOT after removal: \[\]' "$top/out/install-$node.txt" &&
    grep -q 'FASTFETCH\$PRESETS after removal: \[\]' "$top/out/install-$node.txt" &&
    grep -q 'files after removal: \[\]' "$top/out/install-$node.txt" &&
    grep -q 'startup after removal: \[\]' "$top/out/install-$node.txt"
