# Development log

One entry per build or run problem, newest last. Triage codes (as in the sibling ports):
- T toolchain
- L libc (CRTL)
- S shell/process
- N network
- O output/terminal
- D dependency

## Phase 0 (2026-10-08)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| T | `probe_c99.c` on IA64: `%CC-W-UNSUPCONVSPEC` for `%zu` | VSI C 7.4 does not support `%zu`; 7.7 on x86 does | Cast to `unsigned long` and use `%lu` (D8) |
| L/O | `fwrite("…", 1, 20, stdout)` came out as 20 one-character lines | The CRTL writes each `fwrite` item as a record on record-oriented SYS$OUTPUT | Build whole lines and emit each with one `fputs` (D3) |
| S | `PRODUCT SHOW PRODUCT */OUTPUT=x` gave `%DCL-W-IVQUAL` | PRODUCT has no `/OUTPUT` qualifier | `DEFINE/USER SYS$OUTPUT x` first |
| O | Every terminal `$GETDVI` item looked like a disk under `vms.sh` | `@x.COM/OUTPUT=log` makes SYS$COMMAND and SYS$OUTPUT disk files | Probe the terminal with `ssh -tt`; detect it from SYS$OUTPUT (D4) |
| – | First `probe.sh ia64` produced an empty log | Not reproduced; most likely the first sftp session created the work directory and raced the DCL run started alongside the x86 one | Reran it; watch for this on fresh nodes |

## Phase 1 (2026-10-08)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| L | Kernel showed "OpenVMS 0" | The CRTL `uname()` returns release "0", the OpenVMS version in `version`, and a mangled, truncated HW_NAME in `machine` | Kernel uses `version` as the release and `ARCH_NAME` as the machine |
| O | Coloured lines broke before the 80th visible column on an ssh terminal | The terminal driver counts escape sequences towards its wrap width | `$QIO IO$M_NOFORMAT` writes when SYS$OUTPUT is a terminal (D10) |
| O | Every logo line began `ESC[31m ESC[0m ESC[34m` | The colour carried over from the previous line was restated before an immediate `$2` | Colours are emitted lazily, before the next visible character |
| S | `ssh -tt host "mcr …"` lost the end of the output (IA64: all of it) | A one-shot ssh session closes before the pseudo-terminal drains | `tools/run.sh` types the commands into a full interactive session and cuts out the output between markers |
| S | In that session the typed commands were ignored | LOGINOUT flushes typeahead, and IA64's LOGIN.COM takes several seconds | Wait for the login to finish before typing (`RUN_LOGIN_WAIT`, default 12 s) |

## Phase 2 (2026-10-08)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| T | No way to name the x86 CPU | No `$GETSYI` item, and VSI C has no inline assembly or CPUID builtin | One clang-compiled file, `src/vms/cpuid_x86.c` (D11) |
| T | Host test build failed to link `ffX86Cpuid` | The WSL host is x86-64 too, so `#ifdef __x86_64` matched there | Guard with `__VMS && __x86_64` |
| T | 15 undefined `ffPrint*` symbols at link time | Editing `tools/gen_mms.py` from Windows dropped its execute bit; the generator never ran, and a `&&` chain hid that | Run tools as `python3 tools/x.py` / `bash tools/x.sh` (CLAUDE.md) |
| – | Packages makes up most of the run time | `LIB$SPAWN` of PRODUCT: 0.45 s on x86, 0.26 s on IA64 (the whole banner is 0.52 s / 0.21 s) | Accepted (D5); a cache is possible later |

## Phase 3 (2026-10-09)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| F | `SYS$LOGIN:[.^.config.fastfetch]` would not resolve | SYS$LOGIN is a plain directory (`USER$ROOT:[username]`), not a rooted logical, so RMS will not merge a relative directory onto it (vms-gzip noted the same) | Build the path from SYS$LOGIN's translation (D12) |
| O | A config warning shared a line with the first banner line on a terminal | `warn()` wrote the prefix, message and newline as three stderr calls, so three records | Format the message first and write it with one call (D3) |
| O | A blank line before the banner, and a message just before it was overwritten | The `$QIO` NOFORMAT text carried its own CR LF; the driver frames records itself, emitting LF only when the cursor is not on a fresh line | Frame each line with zero-length writes carrying FORTRAN carriage control `$` and `+` (D10, amended) |
| – | `-s` drops a config's per-module keys and formats | As upstream: `-s` replaces the module list | Documented in CONFIG.md; tests check that the config's display settings still apply |

## Phase 4 (2026-10-09)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| T | The first `kit.sh ia64` failed after a good build | `kit.sh` redirected `build.sh`'s output into `out/build-<node>.log`, which `build.sh` writes and then checks itself; `grep` refused ("input file is also the output") | `kit.sh` discards `build.sh`'s output; the log stays `build.sh`'s |
| S | Now and then (three times so far, all IA64) `vms.sh` returned an empty log with success: one install check and one test run reported nothing, or 61 failures | The ssh session ended before the procedure started; `vms.sh` gave up without the completion marker and printed nothing | `vms.sh` retries once when no log exists (nothing ran, so a retry is safe), and otherwise fails with "no completion marker"; `installcheck.sh` also reports an empty log |
| T | clang code must avoid VSI clang's `memset` → `OTS$FILL` miscompile (vms-php) | A family rule | `BUILD.COM` compiles `cpuid_x86.c` with `-fno-builtin-memset` |
| T | Does `crtl_init.c`'s `LIB$INITIALIZE` entry run? With VSI clang on x86 it never does (vms-php) | Not affected: it is compiled with VSI C (`#pragma extern_model`) | Verified on both nodes: under `PARSE_STYLE=EXTENDED`, unquoted `--os-format ABC{name}` keeps `ABC`, which only happens if `DECC$ARGV_PARSE_STYLE` was set; under traditional parsing it is `abc` |

## Phase 5 (2026-10-09)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| L | `RMI$_COM`, `COMO`, `CUR`, `PROCS` gave SS$_BADPARAM in Phase 0 | The process-state items need a 4-byte buffer exactly; Phase 0 asked with 8 (probes/probe_rmi2.c) | `rmi32()` in cpuusage_vms.c |
| L | Queues failed with `%X000081AA` (no message text) after counting every queue | `$GETQUI`'s IOSB status is a longword; read as a word (as for `$GETSYI`), `JBC$_NOMOREQUE` lost its facility bits and looked like an error | Longword IOSB |
| L | Generic printer and server queues were not counted as output | They carry neither the PRINTER nor the SERVER flag (generic batch queues do carry BATCH) | Output = every non-batch queue |
| H | `queues_vms.c` failed to compile on VMS only (implicit `FF_VMS_OK`) | The VMS-only code missed `vmsapi.h`; the host build compiles that part out | Include it; host builds cannot replace VMS builds |
| – | CPU usage scale | `RMI$_CPUIDLE` counts 100 ticks per second per CPU, summed over CPUs (an idle 1-CPU IA64 gained 100 in 1 s) | Usage = 1 - idle / (seconds x 100 x CPUs) |
| O | `Command` showed an empty first line for `SEARCH/STATISTICS` output | DCL output can start with a blank line, not only with indentation | Trim leading blank lines and blanks |
| S | Another IA64 run came back with an empty log and no `vms.sh` message; every check then failed | A lost ssh session on a path the new `vms.sh` check does not see | `test.sh` counts the cases' status lines and reruns once if any are missing, else says "session lost" |

## 0.2.1 (2026-10-09)

| Code | Problem | Root cause | Fix |
|------|---------|------------|-----|
| S | The IA64 install check for 0.2.1 first installed the 0.2.0 kit | `kit.sh` failed (a lost session left an empty build log), and the previous kit was still in [.KIT_IA64] | `installcheck.sh` requires the installed program to report kit.env's version; rerun `kit.sh` |
