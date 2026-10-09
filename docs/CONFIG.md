# Configuration

vms-fastfetch reads fastfetch's JSONC configuration format: JSON plus `//` and `/* */` comments and trailing commas. Configs written for fastfetch load as they are. Keys that don't apply on OpenVMS are ignored, and modules OpenVMS can't support report "Not supported on this platform", which is shown only with `--show-errors`.

## Where the config comes from

| Order | Location |
|-------|----------|
| `-c <file>` | that file, in VMS or UNIX syntax |
| `-c <name>` | `FASTFETCH$PRESETS:<name>.jsonc`, a preset (see below) |
| `-c none` | no config |
| otherwise, 1 | the file named by the logical name `FASTFETCH$CONFIG` |
| otherwise, 2 | `[<login>.^.config.fastfetch]config.jsonc`, i.e. upstream's `~/.config/fastfetch/config.jsonc` under your SYS$LOGIN directory |
| otherwise, 3 | `SYS$LOGIN:FASTFETCH.JSONC` |

`fastfetch --gen-config` writes a config for the current modules to location 2, creating the directories. `--gen-config <file>` writes it elsewhere, and `--gen-config-force` overwrites an existing file. Options on the command line override the config.

## Presets

| Preset | What |
|--------|------|
| `all` | every module supported on OpenVMS (CPUUsage adds a 0.2 s sample) |
| `ci` | no logo, no colours, errors shown: for batch logs and scripts |

## Keys understood

```jsonc
{
    "logo": "openvms",                  // or an object:
    "logo": {
        "type": "auto",                 // auto, builtin, small, none, file, data
        "source": "openvms",            // builtin name; file name (file); the text (data)
        "color": { "1": "red", "2": "blue" },
        "padding": { "top": 0, "left": 0, "right": 4 }
    },
    "display": {
        "separator": ": ",
        "color": "blue",                // keys and title; or:
        "color": { "keys": "bold_blue", "title": "red", "output": "default" },
        "key": { "width": 16 },
        "showErrors": false,
        "pipe": false,
        "size": { "binaryPrefix": "iec", "ndigits": 2 },   // iec, si, jedec
        "percent": { "green": 50, "yellow": 80 }
    },
    "modules": [
        "title",
        { "type": "separator", "string": "-" },
        { "type": "os", "key": "System", "format": "{name} {version} ({arch})",
          "keyColor": "yellow", "outputColor": "default" },
        { "type": "custom", "format": "{#cyan}any text{#}" },
        "disk"
    ]
}
```

Module-specific keys:
- `title`: `fqdn`.
- `separator`: `string`, `length`.
- `colors`: `paddingLeft`.
- `cpuusage`: `waitTime` (sample length in ms, default 200).
- `command`: `text`, the DCL command to run. Its output is the value, one line per output line.
  fastfetch's `shell` option does not apply.

A `command` entry can show anything DCL can (this one is `tests/command.jsonc`):

```jsonc
{ "type": "command", "key": "Booted", "text": "write sys$output f$getsyi(\"BOOTTIME\")" }
```

Logo types that draw images (kitty, sixel, chafa, iterm) are not supported; vms-fastfetch warns and uses the builtin logo. Logos are ASCII only, because DECterm and VT terminals are not UTF-8.

**Colours** are given as fastfetch spells them:
- names: `black` … `white`, `default`;
- prefixes: `bold_`, `dim_`, `italic_`, `underline_`, `blink_`, `inverse_`, `light_`/`bright_`;
- raw SGR: `38;5;208`;
- 24-bit: `#ff8000`.

## Format strings

As fastfetch:
- `{1}` or `{name}` inserts an argument, and `{}` the next one.
- `{1:10}` truncates; `{1<10}` and `{1>10}` pad, left- or right-aligned.
- `{?1}…{?}` shows its contents only if argument 1 is not empty, and `{/1}…{/}` only if it is empty.
- `{#red}`, `{#keys}`, `{#title}` and `{#}` change and reset colour.

Each module's arguments follow upstream's list, in upstream's order, so `{3}` means the same as in fastfetch. Values OpenVMS doesn't have are empty, and OpenVMS-only arguments come last. `fastfetch -s <module> --format json` shows every argument by name, in camelCase (`pretty-name` → `prettyName`).

Disk and LocalIp print one line per volume or interface. Their keys are formats too, by default `Disk ({name})` and `Local IP ({ifname})`.

| Module | Arguments (OpenVMS values) |
|--------|----------------------------|
| Title | user-name, host-name, home-dir (SYS$LOGIN), exe-path, user-shell, user-name-colored, at-symbol-colored, host-name-colored, full-user-name, user-id (UIC), pid, cwd |
| OS | sysname, name, pretty-name, id, id-like, variant, variant-id, version, version-id, codename, build-id, arch, kernel-release |
| Host | family, name (SYI$_HW_NAME), version, sku, vendor, serial, uuid |
| Kernel | sysname, release, version, arch, page-size |
| Uptime | days, hours, minutes, seconds, milliseconds, boot-time, years, days-of-year, years-fraction, formatted |
| Packages | all, pcsi. Upstream's numeric positions, one per Linux package manager, do not carry over. |
| Shell | process-name, exe, exe-name, version, pid, pretty-name, exe-path, tty |
| Terminal | process-name, exe, exe-name, pid, pretty-name, version, exe-path, tty; + kind, type, remote |
| TerminalSize | rows, columns, width, height |
| CPU | name, vendor, cores-physical, cores-logical, cores-online, freq-base, freq-max, temperature, core-types, packages, march, numa-nodes, code-name, technology; + hypervisor |
| Memory | used, total, percentage, percentage-bar |
| Swap ("Page file") | used, total, percentage, percentage-bar, name |
| Disk | size-used, size-total, size-percentage, files-used, files-total, files-percentage, is-external, is-hidden, filesystem, name (volume label), is-readonly, create-time, …, mountpoint (logical volume), mount-from (device), …, size-free, size-available |
| LocalIp | ipv4, ipv6, mac, ifname, is-default-route, mtu, speed, flags |
| Locale | result |
| Processes | result, threads; + hidden |
| Users | name, host-name, session-name, client-ip, login-time; + sessions |
| DateTime | year, year-short, month, month-pretty, month-name, month-name-short, week, weekday, weekday-short, day-in-year, day-in-month, day-in-week, hour, hour-pretty, hour-12, hour-12-pretty, minute, minute-pretty, second, second-pretty, offset-from-utc, timezone-name, day-pretty, am-pm |
| Version | project-name, version, version-tweak, build-type, sysname, arch, cmake-built-type, compile-time, compiler, libc |
| Cluster (OpenVMS only) | member, nodes, votes, quorum |
| CPUUsage | avg, max, max-index, min, min-index, avg-bar, max-bar, min-bar. All CPUs together, so avg = max = min. |
| Loadavg ("Load") | loadavg1 (= runnable), loadavg2, loadavg3 (empty); + runnable, current, computable, computable-outswapped. Instantaneous: OpenVMS keeps no load averages (D15). |
| DNS | result (servers); + domain |
| Command | result |
| Queues (OpenVMS only) | queues, batch, output, generic, stopped, jobs, executing, pending, holding, timed |

## Command line

`fastfetch -h` lists the options. These are fastfetch's:
- `-c`, `-s`, `-l`, `--logo-type`, `--logo-color-N`, `--logo-padding-*`
- `-j`/`--json`/`--format json`, `--pipe`, `--show-errors`, `--separator`, `--key-width`
- `--color`, `--color-keys`, `--color-title`, `--color-output`
- `--size-binary-prefix`, `--size-ndigits`
- `--command-text` (the Command module's DCL)
- per module: `--<module>-key`, `--<module>-format`, `--<module>-key-color`, `--<module>-output-color`
- `--gen-config[-force]`, `--list-modules`, `--list-logos`

DCL upper-cases unquoted arguments, and the C library then lower-cases them, so options and module names are case-insensitive. **Quote format strings** to keep their case and spaces:

```
$ fastfetch -s os:disk --disk-format "{size-percentage} of {size-total} ({filesystem})"
```
