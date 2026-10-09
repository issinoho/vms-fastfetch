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
**Status:** approved by the user (2026-10-08).

Under `_POSIX_EXIT`, `exit(1)` becomes `%X35A00x9`, which has success severity, so DCL sees a failed run as a success. vms-gzip works around this with a GPL `vms_exit.c`. Without `POSIX_EXIT`, the CRTL defines `EXIT_FAILURE` as `%X10000002` (error, message inhibited), and `$STATUS` and `ON ERROR` work as they should in DCL. fastfetch only ever exits 0 or 1, so `main` returns `EXIT_SUCCESS` or `EXIT_FAILURE`.

Verified in Phase 1: an unknown option exits with `$STATUS` `%X10000002` on both nodes.

## D10. Terminal output through `$QIO` with `IO$M_NOFORMAT`
**Status:** approved by the user (2026-10-08).

The VMS terminal driver wraps lines at the terminal width, and it counts escape sequences as printable characters. With colour on, the Host line (`QEMU Standard PC (Q35 + ICH9, 2009)`, 41 visible characters plus 25 bytes of escapes beside a 30-column logo) was broken after "ICH9," on an 80-column ssh terminal.

When SYS$OUTPUT is a terminal, `ffPrintFinish` therefore assigns a channel to it and writes each line's text with `$QIOW IO$_WRITEVBLK|IO$M_NOFORMAT`. This bypasses the driver's wrap and formatting, and the terminal emulator wraps genuinely long lines itself, as on Unix. Files, mailboxes and pipes still go through `fputs` (D3). If a `$QIO` fails, output falls back to `stdout`.

**Amended in Phase 3 (an implementation detail; the decision is unchanged):** the line framing comes from the driver.
- **Before:** the text carried its own CR LF. That gave a blank line before the banner, and the first line overwrote any message printed just before it.
- **Why:** the driver frames records itself. It sends a line feed before a record only when the cursor is not already on a fresh line, and a carriage return after it.
- **Now:** each line is a zero-length formatted write with FORTRAN carriage control `$` (new line), then the text with NOFORMAT, then a zero-length write with `+` (carriage return). The output is byte-for-byte the same as DCL's, in sessions, after config warnings and inside command procedures.

**Alternatives rejected:**
- Changing the user's terminal settings (`SET TERMINAL/NOWRAP`).
- Truncating lines to fit the driver's count of the width.

## D11. x86 CPUID through one clang-compiled file
**Status:** approved by the user (2026-10-09).

On x86-64 no other source names the CPU model: `$GETSYI` has no item for it, `SHOW CPU/FULL` doesn't show it, and VSI C has neither inline assembly nor a CPUID builtin.
- **The exception:** `src/vms/cpuid_x86.c` (one function, `ffX86Cpuid`) is compiled by BUILD.COM with VSI's clang (`SYS$SYSTEM:CLANG.EXE`, part of VSI C++, already on the build node). Everything else stays VSI C (D1).
- **Linking:** VSI C passes its 32-bit pointer sign-extended, which is a valid 64-bit address.
- **IA64:** needs no clang, because VSI C's `__getIndReg` builtin reads the CPUID registers.
- **Without clang:** a kit built on a node without clang would need a fallback. That is not needed here; the binary kit (Phase 4) is built on our node.

**Alternatives rejected:**
- VSI's X86ASM assembler: another toolchain and syntax for six instructions.
- Showing only "x86-64": loses the most-wanted line of the banner.

## D12. Config locations
**Status:** approved by the user (2026-10-09).

Search order when `-c` is not given:
1. the logical name `FASTFETCH$CONFIG`;
2. upstream's `~/.config/fastfetch/config.jsonc`, as `[<login>.^.config.fastfetch]config.jsonc` (ODS-5 `^.` for the leading dot);
3. the VMS-style `SYS$LOGIN:FASTFETCH.JSONC`.

Other `-c` forms:
- `-c <name>` also tries `FASTFETCH$PRESETS:<name>.jsonc`, which the kit will define;
- `-c none` loads nothing.

Location 2 is built from SYS$LOGIN's translation (`USER$ROOT:[username]` → `USER$ROOT:[username.^.config.fastfetch]`). `SYS$LOGIN:[.x]` is not valid on these nodes, because SYS$LOGIN is a plain directory, not a rooted logical. `--gen-config` creates the directories.

## D13. Typed module arguments drive both the text and the JSON output
**Status:** approved by the user (2026-10-09).

Each module reports its result once, as a list of named, typed arguments in upstream's order:
- **Types:** string, count, size, percent, GHz, boolean, plus pre-coloured text.
- **Text output:** a format string over the list. Each module's default layout is itself a format string, so `--<module>-format` and config `format` replace it exactly as in fastfetch.
- **`--format json`:** the same list, with camelCase names (`pretty-name` → `prettyName`) and typed values (bytes, numbers, booleans). Multi-instance modules (Disk, LocalIp) give an array.

These names are close to, but not guaranteed identical to, upstream's JSON field names. Upstream builds JSON from separate C structures, and its JSON for OpenVMS would not exist anyway. Pre-coloured text (Title's coloured parts) is left out of the JSON.

**Alternative rejected:** a hand-written JSON function per module, mirroring upstream's field names. That doubles each module's code and still differs wherever upstream's structures have no OpenVMS data.

## D14. PCSI product FASTFETCH, logicals FASTFETCH$ROOT and FASTFETCH$PRESETS
**Status:** approved by the user (2026-10-09).

The kit is `ISSINOHO <base> FASTFETCH V0.1-0`, installed under `[FASTFETCH]` as the sibling kits are:
- `[.BIN]FASTFETCH.EXE`
- `[.PRESETS]ALL.JSONC`, `CI.JSONC`
- `[.DOC]` README.VMS, CONFIG.MD and the licences
- `[000000]FASTFETCH$SETUP.COM`, which defines the `fastfetch` command
- `SYS$STARTUP:FASTFETCH$STARTUP.COM`, which defines `FASTFETCH$ROOT` (concealed) and `FASTFETCH$PRESETS`, and deassigns both at `PRODUCT REMOVE`

The product name has no VMS prefix (unlike VMSCURL and VMSMARIADB), because VSI ships no product called FASTFETCH. The PCSI version follows the program: 0.1.0 becomes V0.1-0. `vms/kit/kit.env` holds it, and `tools/kit.sh` refuses to build if it disagrees with `FF_PROGRAM_VERSION`.

## D15. Loadavg shows runnable processes now, not averages
**Status:** proposed.

OpenVMS keeps no load averages. `$GETRMI` (MONITOR's data) gives instantaneous process-state counts: `RMI$_CUR` (running), `RMI$_COM` (computable, waiting for a CPU) and `RMI$_COMO` (computable but outswapped).
- **The module:** fastfetch's Loadavg, keyed "Load". It shows `2 runnable now (2 running, 0 waiting)`.
- **The arguments:** `loadavg1` is the runnable count, so `{1}` formats show a number. `loadavg2` and `loadavg3` stay empty rather than repeating it.
- **Not in the default layout**, as upstream.

**Alternative rejected:** sampling for a minute to build averages, which is far too slow for a banner.
