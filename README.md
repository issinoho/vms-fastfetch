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
| 1. Core engine + Title/OS/Host/Kernel/Uptime/Colors | – | – |
| 2. Tier-1 detections (CPU, Memory, Disk, LocalIp, Packages, Terminal, Cluster, …) | – | – |
| 3. Config file and JSON compatibility | – | – |
| 4. PCSI kit and release | – | – |

The probe findings that shape the detection code are in [docs/PROBES.md](docs/PROBES.md).

## Workflow

```sh
cp tools/nodes.conf.example tools/nodes.conf   # then fill in the real nodes
tools/probe.sh <ia64|x86>                      # Phase 0 probes -> out/probes-<node>.log
tools/push.sh <node>                           # upload src/, vms/, probes/ (changed files only)
tools/vms.sh <node> dcl '<cmd>' ...            # run DCL; also put/get/run/batch
```

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
