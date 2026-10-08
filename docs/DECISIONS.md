# Design decisions

Each entry stays *proposed* until the user approves it. Evidence for D3–D8 is in
[PROBES.md](PROBES.md).

## D0. A compatible C99 rewrite, not a port of upstream fastfetch
**Status:** approved by the user (2026-10-08).

**Evidence:** upstream fastfetch 2.70.0 (MIT, HEAD 1e0ec10) sets `CMAKE_C_STANDARD 23` and needs GCC 13 or Clang 16.
- It uses `nullptr` 2,742 times and `[[attribute]]` syntax 823 times.
- It relies on about 1,000 scope-exit cleanup variables built on `[[gnu::cleanup]]`: `FF_STRBUF_AUTO_DESTROY` ×543, `FF_AUTO_CLOSE_FD` ×143, `FF_AUTO_FREE` ×133, `FF_LIST_AUTO_DESTROY` ×75 and others.
- VSI C has no cleanup attribute, and IA64 has no clang. Removing those uses means rewriting most functions, and redoing that on every upstream release.

**Decision:** write our own C99 code that follows fastfetch's design and keeps what users see:
- the `detection/` and `modules/` split;
- module names and the default layout;
- the JSONC config, format strings and `--format json`;
- the logo `.txt` format with `$1..$9` colours.

yyjson and the upstream logo files (both MIT) are reused, with attribution in NOTICE.

## D1. VSI C on x86-64 and IA64, one source tree
**Status:** approved by the user (2026-10-08).

As in vms-curl, vms-gzip and vms-lighttpd: one source tree, the same qualifiers on both architectures (`vms/config/ccflags.txt`), and objects in `[.OBJ_<arch>]`. clang is not used, so IA64 is a first-class target.

## D2. MIT licence; no code copied from the GPL sibling ports
**Status:** approved by the user (2026-10-08).

fastfetch and yyjson are MIT, so the project is MIT too. vms-gzip's `vms_crtl_init.c` and `vms_exit.c` are GPL-3 (they are part of gzip's port), so we write our own versions of these small helpers. The shell tooling in `tools/` is the user's own and is reused.

## D3. The printing engine writes whole lines
**Status:** approved by the user (2026-10-08).

`fwrite(p, 1, n, stdout)` writes one record per item, so each byte becomes a line on SYS$OUTPUT. `printf` holds a line together until `\n`. Every output line, logo column included, is built in an `FFstrbuf`-style buffer and written with a single `fputs`. No `fwrite` with item size 1 and no `write()` per fragment.

## D4. Terminal = SYS$OUTPUT; colour from the device class
**Status:** approved by the user (2026-10-08).

In an interactive ssh session SYS$OUTPUT is the `FTAnnn:` pseudo-terminal, while SYS$COMMAND can be a disk inside a command procedure. ssh terminals report `DEVTYPE` 0 with every `TT_ANSICRT`/`TT_DECCRT*` flag clear, even though the client renders ANSI colour.

- **Colour** is on when SYS$OUTPUT is DC$_TERM, unless `--pipe true` or `NO_COLOR` is given.
- **Width** comes from `DVI$_DEVBUFSIZ`.
- **The Terminal module** names the port class from the device prefix: FTA = pseudo-terminal/ssh, TNA = telnet, LTA = LAT, RTA = SET HOST, OPA0 = console.

## D5. Packages from `PRODUCT SHOW PRODUCT`, not from counting database files
**Status:** approved by the user (2026-10-08).

On IA64 there are 82 `*.PCSI$DATABASE` files but only 55 installed products. Spawning `PRODUCT SHOW PRODUCT *` into a temporary file (`LIB$SPAWN`, `DEFINE/USER SYS$OUTPUT`) takes 0.14–0.42 s and gives the true count, plus producers for a breakdown. If the time matters, add a result cache keyed on the newest database-file timestamp later.

## D6. LocalIp from `getifaddrs()`
**Status:** approved by the user (2026-10-08).

`getifaddrs()` works with VSI TCP/IP V6.0 on both nodes. Only VSI TCP/IP is supported; MultiNet and TCPware are untested.

## D7. Memory from `$GETSYI` + `$GETRMI`
**Status:** approved by the user (2026-10-08).

- Total is `SYI$_MEMSIZE × SYI$_PAGE_SIZE`.
- Used is `MEMSIZE − RMI$_FRLIST − RMI$_MODLIST`, which is what `SHOW MEMORY` shows as in use. `$GETSYI` has no free-page item.
- Swap shows page and swap files together (`SYI$_PAGEFILE_*`, `SYI$_SWAPFILE_*`), with the key "Page file".

## D8. No `%zu`
**Status:** approved by the user (2026-10-08).

VSI C 7.4 on IA64 warns `UNSUPCONVSPEC` on it. Use `%lu` with a cast; `PRIu64` from `<inttypes.h>` works on both.

## D9. No `/MAIN=POSIX_EXIT`; exit with `EXIT_SUCCESS`/`EXIT_FAILURE`
**Status:** proposed.

Under `_POSIX_EXIT`, `exit(1)` becomes `%X35A00x9`, which has success severity, so DCL sees a failed run as a success. vms-gzip works around this with a GPL `vms_exit.c`. Without `POSIX_EXIT`, the CRTL defines `EXIT_FAILURE` as `%X10000002` (error, message inhibited), and `$STATUS` and `ON ERROR` work as they should in DCL. fastfetch only ever exits 0 or 1, so `main` returns `EXIT_SUCCESS` or `EXIT_FAILURE`.

Verified in Phase 1: an unknown option exits with `$STATUS` `%X10000002` on both nodes.

## D10. Terminal output through `$QIO` with `IO$M_NOFORMAT`
**Status:** proposed.

The VMS terminal driver wraps lines at the terminal width, and it counts escape sequences as printable characters. With colour on, the Host line (`QEMU Standard PC (Q35 + ICH9, 2009)`, 41 visible characters plus 25 bytes of escapes beside a 30-column logo) was broken after "ICH9," on an 80-column ssh terminal.

When SYS$OUTPUT is a terminal, `ffPrintFinish` therefore assigns a channel to it and writes each line with `$QIOW IO$_WRITEVBLK|IO$M_NOFORMAT` plus an explicit CR LF. This bypasses the driver's wrap and formatting, and the terminal emulator wraps genuinely long lines itself, as on Unix. Files, mailboxes and pipes still go through `fputs` (D3). If a `$QIO` fails, output falls back to `stdout`.

**Alternatives rejected:**
- Changing the user's terminal settings (`SET TERMINAL/NOWRAP`).
- Truncating lines to fit the driver's count of the width.
