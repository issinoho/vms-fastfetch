#!/usr/bin/env bash
# test.sh <node> - regression checks for the built FASTFETCH.EXE on <node>.
#
# Runs each case through vms.sh (output to a log file, so SYS$OUTPUT is not
# a terminal and pipe mode is automatic) and checks the output here.
# Terminal behaviour (colour, the $QIO writer) is checked by hand with
# tools/run.sh.  Prints "TEST: N passed, M failed"; exits 1 on any failure.
set -uo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: test.sh <node>}
read -r _ ARCH _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
exe="[.BIN_${ARCH}]FASTFETCH.EXE"

# name | arguments
cases=(
  "default|"
  "structure|-s os:uptime -l none"
  "forcecolor|--pipe false -s os -l none"
  "badopt|--bogus"
  "version|-v"
  "unsupported|-s gpu:os --show-errors -l none"
  "hidden|-s gpu:os -l none"
  "linuxlogo|-l linux -s os"
  "uppercase|-S OS:UPTIME -L NONE"
  "badmodule|-s os:nosuchmodule -l none"
)

cmds=("ff = \"\$\" + f\$parse(\"$exe\")")
for c in "${cases[@]}"; do
    name=${c%%|*} args=${c#*|}
    cmds+=("write sys\$output \"=== $name\"" "define/user sys\$error sys\$output" "ff $args"
           "write sys\$output \"=== status \", \$status")
done
mkdir -p "$top/out"
log=$top/out/test-$node.log
"$top/tools/vms.sh" "$node" dcl "${cmds[@]}" > "$log"

# section <name>: the output of one case (without the marker lines)
section() { awk -v n="=== $1" '$0==n{on=1; next} /^=== status /{if(on) exit} on' "$log"; }
status() { awk -v n="=== $1" '$0==n{on=1; next} on && /^=== status /{print $3; exit}' "$log"; }

pass=0 fail=0
check() {  # check <description> <command...>
    if "${@:2}" >/dev/null 2>&1; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1"; fi
}
has() { section "$1" | grep -aqF -- "$2"; }
lacks() { ! section "$1" | grep -aqF -- "$2"; }
lines() { [ "$(section "$1" | wc -l)" -eq "$2" ]; }
esc() { section "$1" | grep -aq $'\033'; }
noesc() { ! esc "$1"; }
first() { section "$1" | head -1 | grep -aqF -- "$2"; }

check "default: title has @"            has default "@"
check "default: OS line"                has default "OS: OpenVMS "
check "default: Host line"              has default "Host: "
check "default: Kernel line"            has default "Kernel: OpenVMS "
check "default: Uptime line"            has default "Uptime: "
check "default: logo"                   has default "\\___/| .__/"
check "default: no escapes (log file)"  noesc default
check "default: success status"         [ "$(status default)" = "%X00000001" ]
check "structure: exactly 2 lines"      lines structure 2
check "structure: OS first"             first structure "OS: "
check "forcecolor: escapes present"     esc forcecolor
check "badopt: error status"            [ "$(status badopt)" = "%X10000002" ]
check "badopt: message"                 has badopt "unknown option"
check "version: text"                   has version "vms-fastfetch "
check "unsupported: shown with errors"  has unsupported "GPU: Not supported on this platform"
check "hidden: not shown by default"    lacks hidden "GPU"
check "hidden: OS still printed"        has hidden "OS: OpenVMS"
check "linuxlogo: 12 logo lines"        lines linuxlogo 12
check "uppercase: options and modules"  lines uppercase 2
check "badmodule: warning"              has badmodule "unknown module"
check "badmodule: OS still printed"     has badmodule "OS: OpenVMS"

echo "TEST: $pass passed, $fail failed ($node)"
[ "$fail" -eq 0 ]
