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
