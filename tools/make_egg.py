#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""GLOUPI BREAKS' easter egg (LOAF LORD): a picture -> app/egg_data.h, full screen, grey.

  tools/make_egg.py art/loaf_lord.webp app/egg_data.h [--size 240] [--levels 16] [--preview OUT.png]

The picture is scaled to fit SIZE x SIZE (centred, on black where it is transparent), its greys reduced to LEVELS (2..16, black to white,
dithered) and run-length coded a byte a run: bits 0-3 the grey, bits 4-7 the run's length less one (1..16 pixels),
rows one after another, no run crossing a row. Needs Pillow.
"""
import argparse
from pathlib import Path

from PIL import Image


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("src", type=Path)
    ap.add_argument("header", type=Path)
    ap.add_argument("--size", type=int, default=240)
    ap.add_argument("--levels", type=int, default=16)
    ap.add_argument("--preview", type=Path)
    a = ap.parse_args()
    n = max(2, min(16, a.levels))
    src = Image.open(a.src).convert("RGBA")
    src.thumbnail((a.size, a.size), Image.LANCZOS)
    img = Image.new("RGBA", (a.size, a.size), (0, 0, 0, 255))   # its transparent parts, and the margins: black
    img.alpha_composite(src, ((a.size - src.width) // 2, (a.size - src.height) // 2))
    img = img.convert("L")
    pal = Image.new("P", (1, 1))
    greys = [round(i * 255 / (n - 1)) for i in range(n)]
    pal.putpalette([v for g in greys for v in (g, g, g)] + [0, 0, 0] * (256 - n))
    q = img.convert("RGB").quantize(palette=pal, dither=Image.Dither.FLOYDSTEINBERG)
    q = q.point(lambda i: i if i < n else 0)
    px = q.load()
    data = bytearray()
    for y in range(a.size):
        x = 0
        while x < a.size:
            c, r = px[x, y], 1
            while x + r < a.size and px[x + r, y] == c and r < 16:
                r += 1
            data.append((r - 1) << 4 | c)
            x += r
    pal565 = ", ".join(f"0x{((g >> 3) << 11) | ((g >> 2) << 5) | (g >> 3):04X}u" for g in greys)
    body = ",".join(("\n    " if i % 24 == 0 else " ") + f"0x{b:02X}" for i, b in enumerate(data))
    a.header.write_text(
        "/* SPDX-License-Identifier: MPL-2.0 */\n"
        f"/* GLOUPI BREAKS' easter egg, made by tools/make_egg.py from {a.src.name}: {a.size} x {a.size}, {n} greys, "
        f"{len(data)} bytes. Regenerate it, don't edit it. */\n"
        "#pragma once\n#include <stdint.h>\n"
        f"#define EGG_W {a.size}\n#define EGG_H {a.size}\n"
        f"static const uint16_t EGG_PAL[{n}] = {{{pal565}}};\n"
        f"static const uint8_t EGG_RLE[{len(data)}] = {{{body}\n}};\n")
    if a.preview:
        prev = Image.new("L", (a.size, a.size))
        prev.putdata([greys[px[x, y]] for y in range(a.size) for x in range(a.size)])
        prev.save(a.preview)
    print(f"{a.header}: {a.size} x {a.size}, {n} greys, {len(data)} bytes")


if __name__ == "__main__":
    main()
