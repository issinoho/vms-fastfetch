# CLAUDE.md

Guidance for working in this repository: vms-fastfetch is a fastfetch-compatible
system-information tool for OpenVMS (IA64 and x86-64). It is an original C99 program, not a
patched upstream tree (docs/DECISIONS.md D0). Read README.md first.

## Ground rules

- **C99 for VSI C on both architectures.** That rules out several things:
  - GCC extensions: cleanup attributes, statement expressions, `typeof`, `__builtin_*`;
  - C11/C23 features;
  - `%zu` (D8).
  Guard VMS-specific headers so that later host-side unit tests stay possible.
- **Output goes through the line-buffer printer, never byte-wise `fwrite`** (D3).
- **Every detection returns `NULL` or an error string.** An unsupported module prints a
  clean "Not supported on this platform", never a crash or a half-filled line.
- **Follow fastfetch's user-facing names and formats** (module names, config keys,
  format-string arguments) so upstream configs and docs apply.
  - Upstream's `doc/json_schema.json` and `src/modules/*/option.h` give the shape.
  - Don't copy upstream C source; the logos and yyjson are the only borrowed files (NOTICE).
- **No GPL code**, including from the sibling vms-* ports (D2).
- **Committed files must not contain real node details.**
  - Use `<ia64-host>`, `<x86-host>` and `DISK$USER:[USERNAME.VMS_FASTFETCH]`.
  - The real values live only in the git-ignored `tools/nodes.conf` and `out/` logs.
  - Redact host names, IPs and serial numbers when copying probe output into `docs/`.
- **Decisions** go in docs/DECISIONS.md, which stays *proposed* until the user approves.
  Build/run problems go in docs/PORTING_LOG.md.
- **Work phase by phase** (README status table) and stop for review at each phase exit.

## Commands

```sh
tools/build.sh <ia64|x86> [ALL|CLEAN]   # push + MMS build; fails on %CC-E/F, link errors
tools/test.sh <node>                    # regression checks, "TEST: N passed, 0 failed"
tools/run.sh <node> [args]              # interactive terminal session (colour, $QIO path)
tools/gen_logos.py; tools/gen_mms.py    # regenerate committed logo_builtin.c / sources.mms
tools/probe.sh <ia64|x86>               # push + run [.PROBES]PROBE.COM, log in out/
tools/push.sh <node>                    # upload src/ vms/ probes/ (changed files; PUSH_ALL=1 for all)
tools/vms.sh <node> dcl '<cmd>' ...     # run DCL; also run/batch/put/get
```

## VMS and tooling pitfalls

- **Use `tools/vms.sh`, never raw `ssh host cmd`.** The exception is `ssh -tt`, which is
  needed to see real terminal behaviour (SYS$OUTPUT = FTAnnn:); wrap it in `timeout`.
- **Terminal output differs from log output** (D10: `$QIO` NOFORMAT on terminals).
  `tools/test.sh` covers only the log path; check terminal changes with `tools/run.sh`
  on both nodes.
- **Under vms.sh**, SYS$COMMAND and SYS$OUTPUT are disk files, and `isatty(1)` is 0.
- **Never use `WAIT` in DCL run over ssh.**
- **`PRODUCT` has no `/OUTPUT`**; use `DEFINE/USER SYS$OUTPUT file`.
- **The CRTL is ILP32 with 32-bit pointers.** `$GETSYI` items can be byte, word, longword
  or quadword (`MEMSIZE` is 8 bytes): zero the buffer and honour the returned length.
- **Some `$GETSYI` items exist on only one architecture** (`HP_ACTIVE_CPU_CNT` returns
  `%X2DC2` on x86). Treat a failed item as "unknown", not as an error.
- **Don't edit a bash script while it is running.** Don't use `pkill -f`.

## Commits

Commit in logical steps with messages that explain the VMS reason. Don't push without the
user asking. Use `git -c user.name="Iain Smith" -c user.email="iain@issinoho.com"` (WSL git
has no identity). Branch `main`; remote (once created) github.com/issinoho/vms-fastfetch.
