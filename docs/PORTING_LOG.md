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
