#!/usr/bin/env bash
# test.sh <node> - regression checks for the built FASTFETCH.EXE on <node>.
#
# Runs each case through vms.sh (output to a log file, so SYS$OUTPUT is not
# a terminal and pipe mode is automatic) and checks the output here.
# Terminal behaviour (colour, the $QIO writer) is checked by hand with
# tools/run.sh.  Prints "TEST: N passed, M failed"; exits 1 on any failure.
#
# The --gen-config default-location case redirects SYS$LOGIN (process
# logical, this session only) to [.TESTS.HOME], so the real login
# directory is never touched.
set -uo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: test.sh <node>}
read -r _ ARCH _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
exe="[.BIN_${ARCH}]FASTFETCH.EXE"

cmds=("ff = \"\$\" + f\$parse(\"$exe\")"
      "top = f\$environment(\"DEFAULT\")"
      "pdir = top - \"]\" + \".PRESETS]\""
      "define/process fastfetch\$presets 'pdir'")

# addcase <name> <arguments> [DCL setup commands...]
addcase() {
    local name=$1 args=$2
    shift 2
    cmds+=("$@" "write sys\$output \"=== $name\"" "define/user sys\$error sys\$output" "ff $args"
           "write sys\$output \"=== status \", \$status")
}

# Phase 1
addcase default ""
addcase structure "-s os:uptime -l none"
addcase forcecolor "--pipe false -s os -l none"
addcase badopt "--bogus"
addcase version "-v"
addcase unsupported "-s gpu:os --show-errors -l none"
addcase hidden "-s gpu:os -l none"
addcase linuxlogo "-l linux -s os"
addcase uppercase "-S OS:UPTIME -L NONE"
addcase badmodule "-s os:nosuchmodule -l none"
# Phase 2
addcase session "-s processes:users:datetime:version -l none"
addcase cluster "-s cluster -l none"
addcase planned "-s battery -l none --show-errors"
# Phase 3
addcase config "-c [.tests]sample.jsonc --show-errors"
addcase configunix "-c tests/sample.jsonc -s os -l none"
addcase preset "-c ci"
addcase presetall "-c all -l none"
addcase osformat "-s os -l none --os-format \"{name}|{version}|{arch}\""
addcase diskkey "-s disk -l none --disk-key \"Vol {mountpoint}\" --disk-format \"{size-percentage} {filesystem}\""
addcase json "-j -s title:os:cpu:memory:disk:gpu:break"
addcase si "-s memory -l none --size-binary-prefix si --size-ndigits 1"
addcase genfile "-s os:kernel --gen-config-force [.tests]gen.jsonc"
addcase usegen "-c [.tests]gen.jsonc -l none"
addcase genexists "-s os --gen-config [.tests]gen.jsonc"
addcase genhome "-s os:uptime --gen-config-force" \
    "hdir = top - \"]\" + \".TESTS.HOME]\"" \
    "if f\$search(top - \"]\" + \".TESTS]HOME.DIR\") .eqs. \"\" then create/directory 'hdir'" \
    "define/process sys\$login 'hdir'"
addcase readhome "-l none"
addcase confignone "-c none -l none -s os" "deassign/process sys\$login"
# Phase 5
addcase cpuusage "-s cpuusage -l none"
addcase loadavg "-s loadavg -l none"
addcase dns "-s dns -l none"
addcase queues "-s queues -l none"
addcase command "-s command -l none --command-text \"write sys\$output \"\"hello\"\" + f\$fao(\"\"!/\"\") + \"\"second\"\"\""
addcase commandnone "-s command -l none --show-errors"
addcase json5 "-j -s cpuusage:loadavg:dns:queues"
addcase commandcfg "-c [.tests]command.jsonc"

mkdir -p "$top/out"
log=$top/out/test-$node.log
# Every case ends with a status line.  A run without all of them lost its
# ssh session (seen on IA64 now and then, sometimes with an empty log and
# no message); retry the whole run once rather than report every check
# as failed.
ncases=$(printf '%s\n' "${cmds[@]}" | grep -c '^write sys\$output "=== status ')
for attempt in 1 2; do
    bash "$top/tools/vms.sh" "$node" dcl "${cmds[@]}" > "$log"
    [ "$(grep -ac '^=== status ' "$log")" -eq "$ncases" ] && break
    if [ "$attempt" = 2 ]; then
        echo "test: $node: the session was lost twice ($(grep -ac '^=== status ' "$log") of $ncases cases ran); rerun" >&2
        exit 1
    fi
    echo "test: $node: the session was lost; running again" >&2
done

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
matches() { section "$1" | grep -aqE -- "$2"; }
ok() { [ "$(status "$1")" = "%X00000001" ]; }
# Lines in the default run, logo column stripped (it is 29 wide plus padding).
grepdefault() { section default | sed -E 's/^.{29} *//' | grep -aqE -- "$1"; }
jsoncheck() { section json | python3 -c "$1"; }
jsonof() { section "$1" | python3 -c "$2"; }

check "default: title has @"            has default "@"
check "default: OS line"                has default "OS: OpenVMS "
check "default: Host line"              has default "Host: "
check "default: Kernel line"            has default "Kernel: OpenVMS "
check "default: Uptime line"            has default "Uptime: "
check "default: logo"                   has default "\\___/| .__/"
check "default: no escapes (log file)"  noesc default
check "default: success status"         ok default
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

check "default: Packages (pcsi)"        grepdefault 'Packages: [0-9]+ \(pcsi\)$'
check "default: Shell DCL"              has default "Shell: DCL"
check "default: CPU with count and GHz" grepdefault 'CPU: .+ \([0-9]+\) @ [0-9.]+ GHz$'
check "default: Memory usage"           grepdefault 'Memory: [0-9.]+ [KMGT]iB / [0-9.]+ [KMGT]iB \([0-9]+%\)$'
check "default: Page file"              has default "Page file: "
check "default: system disk, ODS"       grepdefault 'Disk \([A-Z0-9_$]+\): .+ \([0-9]+%\) - ODS-[25]$'
check "default: Local IP"               grepdefault 'Local IP \([A-Z]+[0-9]+\): [0-9.]+/[0-9]+$'
check "default: Locale"                 has default "Locale: "
check "default: no Terminal off a tty"  lacks default "Terminal:"
check "default: no Cluster standalone"  lacks default "Cluster:"
check "session: 4 lines"                lines session 4
check "session: Processes"              has session "Processes: "
check "session: Users"                  has session "Users: "
check "session: Date & Time"            has session "Date & Time: 20"
check "session: Version"                has session "Version: vms-fastfetch"
check "cluster: hidden when standalone" lines cluster 0
check "planned: unsupported module"     has planned "Battery: Not supported on this platform"

check "config: success"                 ok config
check "config: custom key and format"   matches config 'System -> +OpenVMS [EV][0-9.A-Z-]+ on (x86_64|IA64)$'
check "config: separator string"        has config "=-=-=-"
check "config: custom module"           has config "--- custom text ---"
check "config: key Up for uptime"       matches config 'Up -> +[0-9]'
check "config: small logo"              has config "\\ V / | |  | |___) |"
check "config: unknown module warning"  has config 'unknown module "nosuchmodule"'
check "config: unsupported shown"       has config "GPU -> "
check "config: SI sizes, 1 digit"       matches config 'Memory -> +[0-9]+\.[0-9] GB / '
check "configunix: UNIX path loads"     ok configunix
check "configunix: separator from config" has configunix "OS -> "
check "preset: ci loads"                ok preset
check "preset: ci has no logo"          first preset "@"
check "preset: ci shows Version"        has preset "Version: vms-fastfetch"
check "presetall: Terminal Size errors hidden" lacks presetall "Terminal Size:"
check "presetall: Date & Time"          has presetall "Date & Time: "
check "osformat: by name"               matches osformat '^OS: OpenVMS\|[EV][0-9.A-Z-]+\|(x86_64|IA64)$'
check "diskkey: key from mountpoint"    matches diskkey '^Vol DISK\$[A-Z0-9_$]+: [0-9]+% ODS-[25]$'
check "json: valid, typed"              jsoncheck 'import json,sys; d=json.load(sys.stdin); t={e["type"]:e for e in d}; assert t["OS"]["result"]["prettyName"].startswith("OpenVMS"); assert isinstance(t["CPU"]["result"]["coresOnline"], int); assert isinstance(t["Memory"]["result"]["total"], int); assert isinstance(t["Disk"]["result"], (list, dict)); assert t["GPU"]["error"]; assert "Break" not in t'
check "json: no escapes"                noesc json
check "si: SI units"                    matches si '^Memory: [0-9]+\.[0-9] GB / [0-9]+\.[0-9] GB'
check "genfile: wrote"                  has genfile "fastfetch: wrote "
check "usegen: modules from the file"   lines usegen 2
check "usegen: Kernel second"           matches usegen '^Kernel: '
check "genexists: refuses to overwrite" has genexists "exists; use --gen-config-force"
check "genexists: error status"         [ "$(status genexists)" = "%X10000002" ]
check "genhome: wrote [.^.config.fastfetch]" matches genhome 'wrote .*\.\^\.config\.fastfetch\]config\.jsonc'
check "readhome: default config used"   lines readhome 2
check "readhome: Uptime from config"    matches readhome '^Uptime: '
check "confignone: ignores configs"     lines confignone 1

check "cpuusage: a percentage"          matches cpuusage '^CPU Usage: [0-9]+%$'
check "loadavg: runnable now"           matches loadavg '^Load: [0-9]+ runnable now \([0-9]+ running, [0-9]+ waiting\)$'
check "dns: name servers"               matches dns '^DNS: [0-9.]+'
check "queues: counts"                  matches queues '^Queues: [0-9]+ \([0-9]+ batch, [0-9]+ output\)'
check "command: two lines"              lines command 2
check "command: key on the first"       first command "Command: hello"
check "command: second line alone"      matches command '^second$'
check "commandnone: needs text"         has commandnone "No command given"
check "commandcfg: JSON-escaped DCL"   matches commandcfg '^Booted: +[0-9]+-[A-Z]{3}-[0-9]{4} '
check "json5: typed"                    jsonof json5 'import json,sys; t={e["type"]:e for e in json.load(sys.stdin)}; assert isinstance(t["CPUUsage"]["result"]["avg"], float); assert isinstance(t["Queues"]["result"]["queues"], int); assert t["DNS"]["result"]["result"]; assert isinstance(t["Loadavg"]["result"]["runnable"], int)'

echo "TEST: $pass passed, $fail failed ($node)"
[ "$fail" -eq 0 ]
