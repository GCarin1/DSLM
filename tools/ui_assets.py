#!/usr/bin/env python3
"""Build the DS interface art from its sources (developer tool, needs Pillow).

Sources (committed):
  assets/dslm-logo.png   the project logo; the splash is a crop of it
  assets/ui/font.txt     the pixel font
  assets/ui/icons.txt    small UI icons

Outputs (committed, so the .nds build needs neither Python nor Pillow):
  src/ui/art_data.c/.h        palette, font and icons as C arrays
  assets/ui/gfx/splash.png    256x192 indexed splash for grit (LZ77, 8bpp)
  assets/ui/gfx/splash.grit   grit flags for it
  assets/ui/icon.png          32x32, 16-colour banner icon for ndstool

Usage: python tools/ui_assets.py [--check]
--check exits 1 when an output would change (run it after editing a source).
"""
import io
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
LOGO = ROOT / "assets" / "dslm-logo.png"
FONT = ROOT / "assets" / "ui" / "font.txt"
ICONS = ROOT / "assets" / "ui" / "icons.txt"
OUT_C = ROOT / "src" / "ui" / "art_data.c"
OUT_H = ROOT / "src" / "ui" / "art_data.h"
OUT_SPLASH = ROOT / "assets" / "ui" / "gfx" / "splash.png"
OUT_GRIT = ROOT / "assets" / "ui" / "gfx" / "splash.grit"
OUT_ICON = ROOT / "assets" / "ui" / "icon.png"

# UI palette: indices 32-47 (0-31, palette banks 0-1, stay free for the libnds
# keyboard and its key highlight; 0 is the transparent/backdrop entry). Colours follow the
# logo: navy-black, neon cyan, the DS shell's white and grey.
UI_COLOURS = [
    # name,     letter, rgb
    ("BG",      "k", (0x0B, 0x10, 0x20)),
    ("PANEL",   "p", (0x13, 0x1B, 0x30)),
    ("PANEL2",  "q", (0x1D, 0x2A, 0x47)),
    ("LINE",    "n", (0x2B, 0x41, 0x70)),
    ("CYAN",    "c", (0x2C, 0xC8, 0xFF)),
    ("CYAN_D",  "d", (0x1A, 0x7F, 0xB8)),
    ("CYAN_L",  "l", (0xA8, 0xEC, 0xFF)),
    ("WHITE",   "w", (0xF2, 0xF5, 0xFA)),
    ("GREY",    "g", (0xC3, 0xC9, 0xD6)),
    ("DIM",     "m", (0x7F, 0x8A, 0xA3)),
    ("RED",     "r", (0xFF, 0x52, 0x66)),
    ("RED_D",   "s", (0xA3, 0x28, 0x3B)),
    ("GREEN",   "e", (0x3F, 0xE3, 0x94)),
    ("AMBER",   "a", (0xFF, 0xC8, 0x4D)),
    ("GREY_D",  "h", (0x5A, 0x65, 0x80)),
    ("BLACK",   "b", (0x00, 0x00, 0x00)),
    ("PULSE",   "u", (0x2C, 0xC8, 0xFF)),  # cyan; animated while generating
]
UI_FIRST = 32
ART_FIRST = 64
ART_COLOURS = 48
SPLASH_ART = (240, 154)  # crop of the logo, scaled to fit above the tagline
SPLASH_AT = (8, 6)


def rgb15(rgb):
    r, g, b = rgb
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def parse_blocks(path):
    blocks, name = [], None
    for raw in path.read_text().splitlines():
        if raw.startswith("#") and name is None or not raw.strip() and name is None:
            continue
        if raw.startswith("== "):
            name = raw[3:]
            blocks.append((name, []))
        elif name is not None and raw.strip():
            blocks[-1][1].append(raw.strip())
    return blocks


def build_font():
    glyphs = {}
    for name, rows in parse_blocks(FONT):
        ch = " " if name == "space" else name
        if len(ch) != 1 or not 32 <= ord(ch) < 127:
            sys.exit(f"font.txt: bad glyph name {name!r}")
        width = len(rows[0])
        if any(len(r) != width for r in rows) or len(rows) > 9 or width > 8:
            sys.exit(f"font.txt: glyph {name!r} must be at most 8 wide and 9 tall, rows of equal width")
        bits = []
        for r in rows + ["." * width] * (9 - len(rows)):
            v = 0
            for i, c in enumerate(r):
                if c == "#":
                    v |= 0x80 >> i
            bits.append(v)
        glyphs[ch] = (width, bits)
    missing = [chr(c) for c in range(32, 127) if chr(c) not in glyphs]
    if missing:
        sys.exit(f"font.txt: missing glyphs {''.join(missing)!r}")
    return [glyphs[chr(c)] for c in range(32, 127)]


def build_icons():
    letters = {l: UI_FIRST + i for i, (_, l, _) in enumerate(UI_COLOURS)}
    icons = []
    for name, rows in parse_blocks(ICONS):
        w = len(rows[0])
        px = []
        for r in rows:
            if len(r) != w:
                sys.exit(f"icons.txt: {name} rows differ in width")
            px += [0 if c == "." else letters[c] for c in r]
        icons.append((name, w, len(rows), px))
    return icons


def build_splash():
    """Logo crop -> 256x192 indexed image whose palette is the full DS palette."""
    logo = Image.open(LOGO).convert("RGB").crop((40, 60, 432, 312))
    art = logo.resize(SPLASH_ART, Image.BOX)
    q = art.quantize(ART_COLOURS, method=Image.MEDIANCUT, dither=Image.NONE)
    art_pal = q.getpalette()[: ART_COLOURS * 3]
    art_rgb = [tuple(art_pal[i * 3: i * 3 + 3]) for i in range(ART_COLOURS)]

    bg = UI_FIRST  # near-black pixels become the UI background
    img = Image.new("P", (256, 192), bg)
    src = q.load()
    dst = img.load()
    for y in range(SPLASH_ART[1]):
        for x in range(SPLASH_ART[0]):
            i = src[x, y]
            dst[SPLASH_AT[0] + x, SPLASH_AT[1] + y] = bg if sum(art_rgb[i]) < 60 else ART_FIRST + i

    palette = [(0, 0, 0)] * 256
    palette[0] = UI_COLOURS[0][2]
    for i, (_, _, rgb) in enumerate(UI_COLOURS):
        palette[UI_FIRST + i] = rgb
    for i, rgb in enumerate(art_rgb):
        palette[ART_FIRST + i] = rgb
    img.putpalette([c for rgb in palette for c in rgb])
    return img, palette


def build_icon():
    """32x32 banner icon: a DS-shell frame, a dark screen and the logo's brain."""
    col = {l: rgb for _, l, rgb in UI_COLOURS}
    img = Image.new("RGB", (32, 32), col["k"])
    d = img.load()
    for y in range(32):
        for x in range(32):
            edge = min(x, y, 31 - x, 31 - y)
            corner = min(x, 31 - x) + min(y, 31 - y) < 3
            if corner:
                d[x, y] = col["k"]
            elif edge == 0:
                d[x, y] = col["g"]
            elif edge == 1:
                d[x, y] = col["w"]
            else:
                d[x, y] = col["p"]
    w, h = 26, 20
    brain = Image.open(LOGO).convert("RGB").crop((186, 88, 285, 167)).resize((w, h), Image.BOX).load()
    ox, oy = (32 - w) // 2, (32 - h) // 2
    ink = {}
    for y in range(h):
        for x in range(w):
            r, _, b = brain[x, y]
            if b > 140 and r < 140:
                ink[x, y] = col["c"]
            elif b > 95 and r < 110:
                ink[x, y] = col["d"]
    for (x, y), c in ink.items():  # drop specks from the logo's screen decorations
        if any((x + dx, y + dy) in ink for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy):
            d[ox + x, oy + y] = c
    return img.quantize(16, dither=Image.NONE)


def c_array(name, ctype, values, per_line=16, fmt="0x{:02X}"):
    out = [f"const {ctype} {name}[{len(values)}] = {{"]
    for i in range(0, len(values), per_line):
        out.append("    " + ", ".join(fmt.format(v) for v in values[i: i + per_line]) + ",")
    out.append("};")
    return "\n".join(out)


def render_c(font, icons, palette):
    hdr = ["/* Generated by tools/ui_assets.py from assets/ui/ and assets/dslm-logo.png. Do not edit. */",
           "#ifndef DSLM_ART_DATA_H", "#define DSLM_ART_DATA_H", "", "#include <stdint.h>", "",
           "/* Palette indices (both screens share one 256-colour palette). */"]
    for i, (name, _, _) in enumerate(UI_COLOURS):
        hdr.append(f"#define COL_{name} {UI_FIRST + i}")
    hdr += ["", "extern const uint16_t art_palette[256];", "",
            "/* Font: ASCII 32-126, 9 rows (7 above the baseline), bit 7 = leftmost pixel. */",
            "#define FONT_FIRST 32", "#define FONT_COUNT 95", "#define FONT_HEIGHT 9", "#define FONT_ASCENT 7",
            "extern const uint8_t font_width[FONT_COUNT];",
            "extern const uint8_t font_rows[FONT_COUNT * FONT_HEIGHT];", ""]
    body = ["/* Generated by tools/ui_assets.py from assets/ui/ and assets/dslm-logo.png. Do not edit. */",
            '#include "art_data.h"', "",
            c_array("art_palette", "uint16_t", [rgb15(c) for c in palette], 8, "0x{:04X}"), "",
            c_array("font_width", "uint8_t", [w for w, _ in font], 19, "{}"), "",
            c_array("font_rows", "uint8_t", [b for _, bits in font for b in bits], 9), ""]
    for name, w, h, px in icons:
        up = name.upper()
        hdr += [f"#define ICON_{up}_W {w}", f"#define ICON_{up}_H {h}",
                f"extern const uint8_t icon_{name}[{w * h}];"]
        body += [c_array(f"icon_{name}", "uint8_t", px, w, "{:2d}"), ""]
    hdr += ["", "#endif", ""]
    return "\n".join(hdr), "\n".join(body)


def png_bytes(img):
    buf = io.BytesIO()
    img.save(buf, "PNG", optimize=True)
    return buf.getvalue()


def main():
    check = "--check" in sys.argv[1:]
    font = build_font()
    icons = build_icons()
    splash, palette = build_splash()
    h, c = render_c(font, icons, palette)
    outputs = {
        OUT_H: h.encode(),
        OUT_C: c.encode(),
        OUT_SPLASH: png_bytes(splash),
        OUT_GRIT: b"# 8bpp bitmap, LZ77, no palette (art_palette holds it)\n-gb -gB8 -gzl -p!\n",
        OUT_ICON: png_bytes(build_icon()),
    }
    def same(p, data):  # a Windows checkout may hold the text outputs with CRLF
        if not p.exists():
            return False
        have = p.read_bytes()
        return have == data or (p.suffix != ".png" and have.replace(b"\r\n", b"\n") == data)

    stale = [p for p, data in outputs.items() if not same(p, data)]
    if check:
        for p in stale:
            print(f"stale: {p.relative_to(ROOT)}")
        return 1 if stale else 0
    for p in stale:
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(outputs[p])
        print(f"wrote {p.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
