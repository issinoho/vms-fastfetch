# vms-fastfetch

A [fastfetch](https://github.com/fastfetch-cli/fastfetch)-compatible system information tool for
OpenVMS (x86-64 and IA64), built natively with VSI C.

This is not a port. Upstream fastfetch needs a C23 compiler and GCC cleanup attributes, which
VSI C does not have (see [docs/DECISIONS.md](docs/DECISIONS.md), D0). vms-fastfetch is a C99
rewrite that follows fastfetch's design and keeps its interface: the same module names and
default layout, JSONC config, format strings, `--format json` and logo format. Its data comes
from OpenVMS system services (`$GETSYI`, `$GETJPI`, `$GETDVI`, `$GETRMI`, `$DEVICE_SCAN`) where
fastfetch reads /proc and sysfs.

## Status

| Phase | IA64 (V8.4-2L3) | x86-64 (E9.2-4) |
|-------|-----------------|-----------------|
| 0. Scaffold and probes (`tools/probe.sh`) | done | done |
| 1. Core engine + Title/OS/Host/Kernel/Uptime/Colors; `tools/test.sh` 21/21 | done | done |
| 2. CPU, Memory, Page file, Disk, LocalIp, Packages, Shell, Terminal, Processes, Users, Locale, DateTime, Version, Cluster; `tools/test.sh` 38/38 | done | done |
| 3. Config file and JSON compatibility | – | – |
| 4. PCSI kit and release | – | – |

The probe findings that shape the detection code are in [docs/PROBES.md](docs/PROBES.md).

## Workflow

```sh
cp tools/nodes.conf.example tools/nodes.conf   # then fill in the real nodes
tools/build.sh <ia64|x86> [ALL|CLEAN]          # push and build with MMS -> [.BIN_<arch>]FASTFETCH.EXE
tools/test.sh <node>                           # regression checks (log-file output)
tools/run.sh <node> [args]                     # run in an interactive terminal session (colours, $QIO)
python3 tools/gen_logos.py                     # after changing src/logo/ (output committed)
python3 tools/gen_mms.py                       # after adding/removing sources (output committed)
tools/probe.sh <node>                          # Phase 0 probes -> out/probes-<node>.log
tools/vms.sh <node> dcl '<cmd>' ...            # run DCL; also put/get/run/batch
```

On VMS, define a foreign command and run it:

```
$ fastfetch :== $DISK$USER:[USERNAME.VMS_FASTFETCH.BIN_X86_64]FASTFETCH.EXE
$ fastfetch
$ fastfetch -s os:host:uptime -l vms_small
```

Options and module names are case-insensitive, because DCL upper-cases unquoted arguments.

## Layout

| Path | What |
|------|------|
| `src/` | the program. `src/3rdparty/yyjson` is yyjson 0.13.0, unmodified. |
| `vms/` | VMS build procedures, compiler flags, (later) kit and smoke test |
| `probes/` | Phase 0 probe programs and `PROBE.COM` |
| `tools/` | host-side scripts (bash, run from WSL/Linux) |
| `docs/` | decisions, probe results, development log |

## Licence

MIT; see [LICENSE](LICENSE). Design, module names, config format and logos follow fastfetch
(MIT), and yyjson is bundled (MIT); see [NOTICE](NOTICE).
