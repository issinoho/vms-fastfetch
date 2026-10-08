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
