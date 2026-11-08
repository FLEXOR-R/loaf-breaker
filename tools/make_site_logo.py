#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""LOAF BREAKER: the site's logo, LOAF LORD's face without its lettering (art/loaf_lord.webp, the face cropped square
above the words), its black let go into the page (alpha from its light), and the browser tab's icon:

  tools/make_site_logo.py        -> app/site/img/loaf-lord.png (384 px), app/site/img/favicon.png (64 px)"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "art" / "loaf_lord.webp"
OUT = ROOT / "app" / "site" / "img"
BOX = (349, 103, 1649, 1403)              # the face, square, above the lettering (it starts at row ~1430 of 2000)


def main():
    im = Image.open(SRC).convert("RGB").crop(BOX)
    lum = im.convert("L").point(lambda v: min(255, v * 4))   # (black: see-through; the face: solid)
    face = im.convert("L").convert("LA")             # (grey and its alpha: half the size of RGBA)
    face.putalpha(lum)
    OUT.mkdir(parents=True, exist_ok=True)
    face.resize((384, 384), Image.LANCZOS).save(OUT / "loaf-lord.png", optimize=True)
    face.resize((64, 64), Image.LANCZOS).save(OUT / "favicon.png", optimize=True)
    print(f"make_site_logo: {OUT / 'loaf-lord.png'}, {OUT / 'favicon.png'}")


if __name__ == "__main__":
    main()
