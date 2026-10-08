# Phase 0 probe results

Run on 2026-10-08 with `tools/probe.sh <node>` (sources in `probes/`). The raw logs are in
`out/probes-<node>.log`. They are git-ignored because they hold host names, addresses and
serial numbers. This file is the redacted summary that the detection code is designed
against.

| | x86-64 | IA64 |
|---|---|---|
| OpenVMS | E9.2-4 | V8.4-2L3 |
| Compiler | VSI C x86-64 V7.7-003 (`__DECC_VER` 70790003) | VSI C V7.4-001 (`__DECC_VER` 70490001) |
| CRTL (`__CRTL_VER`) | 90240000 | 90210000 |
| TCP/IP | VSI TCP/IP V6.0-31 | VSI TCP/IP V6.0-31 |
| Hardware | QEMU/KVM guest, 2 CPUs, 7.09 GB | HP rx2660, 1 CPU, 3.99 GB |

## Compiler and CRTL
- **C99:** `/STANDARD=C99` compiles cleanly on both, and `__STDC_VERSION__` is 199901L. These all work: designated initialisers, compound literals, `inline`, for-loop declarations, flexible array members, `stdbool.h`/`stdint.h`/`inttypes.h` (`PRIu64`), `long long`, `va_copy` and `strtoull`.
- **`snprintf`/`vsnprintf`** are C99-correct: they return the untruncated length.
- **`%zu`:** VSI C 7.4 on IA64 gives `%CC-W-UNSUPCONVSPEC` (x86 accepts it). **Don't use `%zu`**; cast to `unsigned long`.
- **Data model:** ILP32 with 32-bit pointers (`sizeof long` = `sizeof void*` = `sizeof size_t` = 4).
- **yyjson 0.13.0** (official release, identical to the copy fastfetch bundles) compiles cleanly under both `/STANDARD=C99` and the default standard on both architectures. It parses JSONC (comments, trailing commas, uint64 max) and writes it back correctly.
- **Locale:** `setlocale(LC_ALL, "")` returns "C". `strftime %Z` gives the VMS timezone name (BST).

## Output (decision D3)
- **`printf`** keeps a line together until `\n`, including partial `printf` calls and a 300-character line. ANSI SGR escapes, including 256-colour ones, pass through unchanged.
- **`fwrite(p, 1, n, stdout)`** writes **one record per item**, so 20 bytes came out as 20 lines. The same happened on both nodes.
- **`write(1, …)`** writes one record per call.
- **Consequence:** the printing engine builds each output line in a buffer and emits it with one `fputs`/`printf` (or one `fwrite(p, n, 1, …)`).

## Terminal (D4)
- **Interactive `ssh -tt`:** SYS$OUTPUT is the pseudo-terminal `FTAnnn:`, with `DEVCLASS` 66 (DC$_TERM), `DEVTYPE` 0 (unknown), `DEVBUFSIZ` 80 (width) and `TT_PAGE` 24 (rows). `TT_PHYDEVNAM` is `_FTAnnn:` and `TT_ACCPORNAM` is empty.
  - All of `TT_ANSICRT`, `TT_DECCRT*`, `TT_AVO` and `TT_EIGHTBIT` are 0, because ssh terminals are not SET TERMINAL/INQUIRE'd. So the decision to use colour cannot depend on them.
- **Under an `@x.COM/OUTPUT=` log:** SYS$COMMAND and SYS$OUTPUT are disk devices, and `isatty(1)` = 0.
- **Environment:** `TERM` is empty, and `SHELL`, `LANG`, `COLUMNS` and `LINES` are unset. `HOME` is the VMS form (`user$root:[username]`). `USER` is set, and `gethostname()` returns the FQDN.
- **Rule:**
  - The terminal is SYS$OUTPUT.
  - Colour is on when its `DEVCLASS` is DC$_TERM (or `isatty(1)`), unless `--pipe` or `NO_COLOR` is given.
  - Width comes from `DEVBUFSIZ`.
  - The Terminal module reports the port class from the device name: FTA = pseudo-terminal (ssh), TNA = telnet, LTA = LAT, OPA0 = console, RTA = DECnet SET HOST.

## $GETSYI (`<syidef.h>`)
| Item | x86-64 | IA64 | Use |
|---|---|---|---|
| `VERSION` | `E9.2-4` | `V8.4-2L3` | OS, Kernel |
| `ARCH_NAME` | `x86_64` | `IA64` | OS |
| `HW_NAME` | `QEMU Standard PC (Q35 + ICH9, 2009)` | `HP rx2660  (1.59GHz/6.0MB)` (two spaces) | Host. On IA64 the CPU clock and cache are in the text. |
| `BOOTTIME` | quadword VMS time | same | Uptime |
| `ACTIVECPU_CNT` / `AVAILCPU_CNT` | 2 / 2 | 1 / 1 | CPU |
| `CPU_SOCKETS`, `HP_CORE_CNT` | –, 2 | –, 1 | CPU (sockets not probed; add in Phase 2) |
| `CPUTYPE` / `REAL_CPUTYPE` | 0 / 0 | 32 / 32 | IA64 CPU family |
| `MEMSIZE` (pages, len 8) / `PAGE_SIZE` | 930264 / 8192 | 524240 / 8192 | Memory total |
| `AVAIL_PAGES` | (not probed yet) | | Memory, cross-check |
| `PAGEFILE_PAGE`/`_FREE` | 60528 / 60528 | 524744 / 524744 | Swap ("Page file") |
| `SWAPFILE_PAGE`/`_FREE` | 0 / 0 | 6600 / 6600 | Swap |
| `CLUSTER_MEMBER`/`_NODES`/`_VOTES`/`_QUORUM` | 0 (standalone) | 0 | Cluster |
| `VOTES`/`EXPECTED_VOTES` | 1 / 1 | 1 / 1 | Cluster |
| `VIRTUAL_MACHINE` | 1 | 0 | Host ("(virtual)") |
| `SERIAL_NUMBER` | "" | set (redacted) | not shown by default |
| `HP_ACTIVE_CPU_CNT` | `%X2DC2` (unsupported) | 1 | ignore |

**Not defined in `syidef.h` on either node:** `CPU_FREQ`, `CPU_SPEED`, `PROCESSOR_TYPE`, `FREE_PAGES`, `USED_PAGES`, `MODIFIED_PAGES`, `TOTAL_PAGEFILES`, `PRODUCT_NAME` and `HYPERVISOR_*`. No `$GETSYI` item gives an x86 CPU model string.

## $GETRMI (`<rmidef.h>`, what MONITOR uses)
- **`RMI$_FRLIST`** (free list, pages) and **`RMI$_MODLIST`** (modified list) work on both nodes. On IA64, where both were read the same minute, they agree with `SHOW MEMORY` to within 0.1% (210945 against 210874 free pages). So Memory used = `MEMSIZE − FRLIST − MODLIST`, with free and modified as in SHOW MEMORY.
- **`RMI$_CPUIDLE`** is a cumulative idle-tick counter and works on both. It is the basis for CPUUsage (two samples).
- **`RMI$_MODES`** returns a layout that differs between the architectures (x86 returns length 0 with data present). Don't use it.
- **`RMI$_COM`/`COMO`/`CUR`/`PROCS`** return `%X14` (SS$_BADPARAM) when requested singly. Count processes with `$GETJPI` instead.

## $GETJPI
- **Own process:** `USERNAME`, `CLINAME` ("DCL"), `PRCNAM`, `MODE` and `UIC` all work.
- **Wildcard scan** (`pidadr = -1`) as an unprivileged user works on both nodes, with no NOPRIV or SUSPENDED results. x86 counted 49 processes (2 interactive, 19 network); IA64 counted 46 (5 interactive). This drives the Processes and Users modules.

## Disks ($DEVICE_SCAN + $GETDVI)
- **`$DEVICE_SCAN`** with `DVS$_DEVCLASS = DC$_DISK` lists all disks. Filter on `DVI$_MNT`.
  - x86 also lists `DMM0` (an unmounted memory disk), `LDM0`/`LDMnnnn` (LD devices; one is the mounted memory-disk boot image `SYS$MD…`) and unmounted DKA100.
  - IA64 lists DKA800 and a DECnet `DNA0` (not mounted).
- **Per disk:** `LOGVOLNAM` (`DISK$X86SYS`), `VOLNAM`, `MAXBLOCK`, `FREEBLOCKS` (512-byte blocks), `CLUSTER` and `ACPTYPE` (11 = ODS-5 here; to confirm the code mapping for ODS-2) all return values.
- **Default Disk module:** mounted disks except LD memory-disk images (`SYS$MD*` volume), with the system disk first.

## Network (D6)
- **`getifaddrs()`** exists and works on both nodes. It reports IPv4 addresses with the interface name (`IE0`, `WE1`) and flags (`IFF_UP`, `IFF_LOOPBACK`). It reports no AF_LINK/MAC entries; family −1 means no address.
- **`SIOCGIFCONF`** also works, with or without `_SOCKADDR_LEN`, and additionally returns family-18 (AF_LINK) entries.
- **LocalIp** uses `getifaddrs`, skips loopback and down interfaces, and could take the MAC from SIOCGIFCONF later.

## Packages (D5)
- **`PRODUCT SHOW PRODUCT *`** (sent to a file with `DEFINE/USER SYS$OUTPUT`; PRODUCT has no `/OUTPUT`) took 0.42 s on x86 and 0.14 s on IA64. It lists 70 and 55 installed products respectively.
- **Counting `SYS$COMMON:[SYSEXE]*.PCSI$DATABASE`** files takes about 0.1 s.
  - On x86 it gives 73, which is 70 products plus 3 `PCSI$*` files.
  - On IA64 it gives 82, which does not match the 55 products, because the database keeps files for removed and superseded products.
  - So the file count is not reliable. Use the PRODUCT output; the producer column also allows a breakdown such as `70 (pcsi: VSI 50, ISSINOHO 18, …)`.

## Still open
- **x86 CPU model name.** `$GETSYI` has none, VSI C's `<builtins.h>` has no CPUID builtin, and `SHOW CPU/FULL` shows no model. Options: a small x86 assembler routine (the VSI X86ASM kit is installed on the x86 node), or fall back to "x86-64 (2 CPUs, KVM guest)". Decide in Phase 2.
- **IA64 CPU name:** map `CPUTYPE` 32 to an Itanium family name and parse the clock from `HW_NAME`.
- **Real VT terminals:** `TT_DECCRT*` on an actual DECterm or VT emulator. Needs a hand test by the user in Phase 2.
