#!/usr/bin/env python3
"""gen_images.py - generate docs/images/banner.svg and icon.svg.

The style follows the sibling vms-* ports (a DECterm window on the left,
title and architecture keycaps on the right; a VT monitor icon).  The
terminal shows a fastfetch banner from the IA64 node, with a made-up node
name and without the address and terminal lines.  The logo comes from
src/logo/ascii/openvms.txt, so the art matches the program.  The output
is committed; run after changing the logo or the text here.
"""
import pathlib
from xml.sax.saxutils import escape

top = pathlib.Path(__file__).resolve().parent.parent
out_dir = top / "docs" / "images"

SANS = "'DejaVu Sans','Liberation Sans','Helvetica Neue',Arial,sans-serif"
MONO = "'DejaVu Sans Mono','Liberation Mono','Menlo','Consolas','Courier New',monospace"

# ANSI colours as a colour DECterm shows them, on the family's dark screen.
RED, BLUE, VALUE, PROMPT = "#ff6e5e", "#78a8ff", "#efe3cc", "#ffb43d"
GREEN, YELLOW = "#6fdc6f", "#ffd75e"
BLOCKS = ["#2e2e2e", "#d9443a", "#4cb84c", "#d9b53a", "#3f6fd9", "#b54cb8", "#3ab5b8", "#d6d6d6"]

DEFS = """<defs>
<linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">
  <stop offset="0" stop-color="#0d1824"/><stop offset="1" stop-color="#16323f"/></linearGradient>
<linearGradient id="titlebar" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#5a82ad"/><stop offset="1" stop-color="#2d5079"/></linearGradient>
<radialGradient id="glow" cx="0.45" cy="0.45" r="0.75">
  <stop offset="0" stop-color="#ffb43d" stop-opacity="0.08"/><stop offset="1" stop-color="#ffb43d" stop-opacity="0"/></radialGradient>
<pattern id="scan" width="4" height="4" patternUnits="userSpaceOnUse">
  <rect width="4" height="1.4" fill="#000" fill-opacity="0.28"/></pattern>
<pattern id="grid" width="32" height="32" patternUnits="userSpaceOnUse">
  <path d="M32,0 H0 V32" stroke="#ffffff" stroke-opacity="0.035" fill="none"/></pattern>
<filter id="phosphor" x="-5%" y="-20%" width="110%" height="140%">
  <feGaussianBlur stdDeviation="1.1" result="b"/>
  <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
<filter id="soft" x="-10%" y="-10%" width="120%" height="130%">
  <feDropShadow dx="0" dy="6" stdDeviation="8" flood-color="#000" flood-opacity="0.45"/></filter>
</defs>"""


def logo_lines():
    """openvms.txt -> [(colour, text)], following the $1/$2 markers."""
    colours = {"1": RED, "2": BLUE}
    lines, colour = [], RED
    for raw in (top / "src/logo/ascii/openvms.txt").read_text().rstrip("\n").split("\n"):
        if raw[:1] == "$" and raw[1:2] in colours:
            colour = colours[raw[1]]
            raw = raw[2:]
        lines.append((colour, raw))
    return lines


def tspan(text, fill, bold=False):
    weight = ' font-weight="bold"' if bold else ""
    return f'<tspan fill="{fill}"{weight}>{escape(text)}</tspan>'


def info(key, value):
    return [(key, BLUE, True), (": ", VALUE, False), (value, VALUE, False)]


def usage(key, used, total, pct, colour, tail=""):
    return [(key, BLUE, True), (": ", VALUE, False), (f"{used} / {total} (", VALUE, False),
            (f"{pct}%", colour, False), (")" + tail, VALUE, False)]


INFO = [
    [("SYSTEM", RED, True), ("@", VALUE, False), ("ORION", RED, True)],
    [("------------", VALUE, False)],
    info("OS", "OpenVMS V8.4-2L3 IA64"),
    info("Host", "HP rx2660 (1.59GHz/6.0MB)"),
    info("Kernel", "OpenVMS V8.4-2L3"),
    info("Uptime", "7 days, 1 hour"),
    info("Packages", "55 (pcsi)"),
    info("Shell", "DCL"),
    info("CPU", "Intel Itanium 2 9000 series (1) @ 1.59 GHz"),
    usage("Memory", "2.38 GiB", "4.00 GiB", 60, YELLOW),
    usage("Disk (I64SYS)", "10.55 GiB", "33.92 GiB", 31, GREEN, " - ODS-5"),
    [("Queues", BLUE, True), (": ", VALUE, False), ("7 (1 batch, 6 output)", VALUE, False)],
    "BLOCKS",
]


def banner():
    W, H = 1280, 400
    o = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" '
         f'role="img" aria-label="fastfetch for OpenVMS">', DEFS,
         f'<rect width="{W}" height="{H}" fill="url(#bg)"/>',
         f'<rect width="{W}" height="{H}" fill="url(#grid)"/>']
    # The DECterm window (as the sibling banners).
    o.append('<g filter="url(#soft)">')
    o.append('<rect x="40" y="36" width="724" height="330" fill="#c4c4c4"/>')
    o.append('<path d="M40,366 V36 H764" stroke="#f2f2f2" stroke-width="3" fill="none"/>')
    o.append('<path d="M40,366 H764 V36" stroke="#5e5e5e" stroke-width="3" fill="none"/>')
    o.append('<rect x="47" y="43" width="710" height="30" fill="url(#titlebar)"/>')
    for x, glyph in ((50, "min"), (702, "dot"), (729, "max")):
        o.append(f'<rect x="{x}" y="46" width="24" height="24" fill="#c4c4c4"/>'
                 f'<path d="M{x},70 V46 H{x+24}" stroke="#f2f2f2" stroke-width="2" fill="none"/>'
                 f'<path d="M{x},70 H{x+24} V46" stroke="#5e5e5e" stroke-width="2" fill="none"/>')
        if glyph == "min":
            o.append(f'<rect x="{x+5}" y="56" width="14" height="4" fill="#5e5e5e"/>')
        elif glyph == "dot":
            o.append(f'<rect x="{x+9}" y="56" width="6" height="6" fill="#5e5e5e"/>')
        else:
            o.append(f'<rect x="{x+6}" y="51" width="12" height="12" fill="none" stroke="#5e5e5e" stroke-width="2"/>')
    o.append(f'<text x="402.0" y="64" text-anchor="middle" font-family="{SANS}" font-size="15" '
             'font-weight="bold" fill="#ffffff">DECterm 1:  fastfetch</text>')
    o.append('<rect x="47" y="78" width="710" height="281" fill="#5e5e5e"/>')
    o.append('<path d="M47,359 H757 V78" stroke="#f2f2f2" stroke-width="2" fill="none"/>')
    o.append('<rect x="50" y="81" width="704" height="275" fill="#120c04"/>')
    o.append('<rect x="50" y="81" width="704" height="275" fill="url(#glow)"/>')

    # Terminal text: 14 px DejaVu Sans Mono is 8.43 px a column; 80 columns fit.
    size, step, x0, y = 13.6, 17.6, 60, 100
    col = size * 0.6021
    o.append(f'<g font-family="{MONO}" font-size="{size}" filter="url(#phosphor)">')
    o.append(f'<text x="{x0}" y="{y:.1f}" xml:space="preserve">{tspan("$ ", PROMPT)}'
             f'{tspan("fastfetch", PROMPT, True)}</text>')
    logo = logo_lines()
    rows = max(len(logo), len(INFO))
    info_x = x0 + 29 * col          # logo width 25 + padding 4
    for i in range(rows):
        y += step
        if i < len(logo):
            colour, text = logo[i]
            o.append(f'<text x="{x0}" y="{y:.1f}" xml:space="preserve">{tspan(text, colour)}</text>')
        if i < len(INFO):
            if INFO[i] == "BLOCKS":
                for b, c in enumerate(BLOCKS):
                    o.append(f'<rect x="{info_x + b * 3 * col:.1f}" y="{y - size + 2:.1f}" '
                             f'width="{3 * col:.1f}" height="{step - 1:.1f}" fill="{c}"/>')
            else:
                spans = "".join(tspan(t, c, b) for t, c, b in INFO[i])
                o.append(f'<text x="{info_x:.1f}" y="{y:.1f}" xml:space="preserve">{spans}</text>')
    o.append("</g>")
    o.append('<rect x="50" y="81" width="704" height="275" fill="url(#scan)"/>')
    o.append("</g>")

    # Right: the icon's monitor, title, architectures, a summary.
    o.append(f'<g filter="url(#soft)"><g transform="translate(796,52) scale(0.6)">{monitor()}</g></g>')
    o.append(f'<text x="972" y="128" font-family="{SANS}" font-weight="bold" font-size="52" '
             'fill="#ffffff" textLength="262" lengthAdjust="spacingAndGlyphs">fastfetch</text>')
    o.append(f'<text x="974" y="172" font-family="{SANS}" font-size="30" fill="#9fd3ff">for OpenVMS</text>')
    for x, label in ((812, "IA64"), (924, "x86-64")):
        o.append(f'<g><rect x="{x}" y="254" width="96" height="46" rx="7" fill="#a69d86"/>'
                 f'<rect x="{x}" y="248" width="96" height="46" rx="7" fill="#d9d2bf"/>'
                 f'<rect x="{x+5}" y="251" width="86" height="34" rx="5" fill="#f1ecdf"/>'
                 f'<text x="{x+48}.0" y="274" text-anchor="middle" font-family="{SANS}" font-size="17" '
                 f'font-weight="bold" fill="#3b382f">{label}</text></g>')
    o.append(f'<text x="812" y="352" font-family="{SANS}" font-size="16" fill="#8fa6b8">'
             'fastfetch 2.70 compatible  ·  JSONC configs  ·  PCSI kits</text>')
    o.append("</svg>")
    return "\n".join(o) + "\n"


def monitor():
    """The family's VT monitor (256x256), with a small fastfetch on screen."""
    o = ['<rect x="14" y="10" width="228" height="196" rx="22" fill="#d9d2bf"/>',
         '<rect x="14" y="10" width="228" height="196" rx="22" fill="none" stroke="#a69d86" stroke-width="3"/>',
         '<rect x="30" y="24" width="196" height="160" rx="14" fill="#2a2a26"/>',
         '<rect x="38" y="31" width="180" height="146" rx="10" fill="#120c04"/>',
         '<rect x="38" y="31" width="180" height="146" rx="10" fill="url(#glow)"/>',
         f'<g font-family="{MONO}" filter="url(#phosphor)">']
    # "VMS" from the small logo, then three key/value lines.
    small = (top / "src/logo/ascii/openvms_small.txt").read_text().rstrip("\n").split("\n")
    y = 52
    for line in small:
        text = line[2:] if line.startswith("$1") else line
        o.append(f'<text x="58" y="{y}" font-size="11.5" font-weight="bold" xml:space="preserve" '
                 f'fill="{BLUE}">{escape(text)}</text>')
        y += 13
    for key, value, colour in (("OS", "OpenVMS", VALUE), ("CPU", "Itanium", VALUE), ("Mem", "60%", YELLOW)):
        y += 3
        o.append(f'<text x="50" y="{y + 12}" font-size="13" xml:space="preserve">'
                 f'{tspan(key, BLUE, True)}{tspan(": ", VALUE)}{tspan(value, colour)}</text>')
        y += 14
    o.append("</g>")
    o.append('<rect x="38" y="31" width="180" height="146" rx="10" fill="url(#scan)"/>')
    o.append('<rect x="96" y="188" width="64" height="10" rx="2" fill="#4a463c"/>')
    o.append(f'<text x="128" y="196.5" text-anchor="middle" font-family="{SANS}" font-size="8" '
             'font-weight="bold" fill="#e9e4d6" letter-spacing="1.5">OpenVMS</text>')
    o.append('<path d="M78,206 L70,240 H186 L178,206 Z" fill="#c9c1ac"/>')
    o.append('<rect x="56" y="236" width="144" height="12" rx="5" fill="#b3aa93"/>')
    return "\n".join(o)


def icon():
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256" '
            'role="img" aria-label="fastfetch for OpenVMS">\n' + DEFS + "\n" + monitor() + "\n</svg>\n")


out_dir.mkdir(parents=True, exist_ok=True)
(out_dir / "banner.svg").write_text(banner())
(out_dir / "icon.svg").write_text(icon())
print("gen_images: docs/images/banner.svg, docs/images/icon.svg")
