#!/bin/sh
# LOAF BREAKER: the manual's and the site's screenshots, from the host simulator's scenarios (and the update loader's
# preview), copied into app/docs/img under the names the manual uses. Run after ./build.sh --host:
#   sh tools/site_shots.sh
set -e
cd "$(dirname "$0")/.."
H=app/build/host/choppah_host
OUT=app/build/scenarios
IMG=app/docs/img
[ -x "$H" ] || { echo "site_shots: build the host first (./build.sh --host)"; exit 1; }
mkdir -p "$OUT"
for sc in site ui cdj pads seq hits fxviews mbcomp nav kfx kickmenu terror drops track trkgap maxout reset; do
    [ -f "tests/scenarios/$sc.txt" ] && "$H" "tests/scenarios/$sc.txt" "$OUT" >/dev/null 2>&1 || true
done
L=app/build/ldr_shots
mkdir -p "$L"
cc -std=c99 -I tests/loader/src -I tests/loader/boards/fm1 -I app/loader tests/loader/src/preview.c -o "$L/ldr_preview"
(cd "$L" && ./ldr_preview)
python3 - "$OUT" "$IMG" "$L" <<'PY'
import sys
from pathlib import Path
from PIL import Image
out, img, ldr = map(Path, sys.argv[1:4])
MAP = {  # the manual's name: the scenario's shot
    "screen-slicer": "site_cdj", "screen-slices": "site_slices", "screen-start": "u3_intro_full",
    "screen-selhelp": "site_selhelp", "screen-held": "site_held", "screen-keyloop": "u6_held",
    "screen-slicefx": "x_key_fx", "pad-roll8": "p_roll8", "pad-stop": "p_stop", "pad-dub": "p_dub",
    "screen-seq": "q_seq", "screen-step": "h_step", "screen-overlay": "o_page", "screen-fxchain": "n_fx",
    "screen-fx-tape": "v_tape", "screen-fx-more": "n_fx2", "screen-mbcomp": "c_multiband",
    "screen-mbcomp-band": "c_mb_low", "screen-mbcomp-solo": "c_mb_solo", "screen-global": "n_glo",
    "screen-glomore": "n_glo3", "screen-lobotomized": "site_lobo", "screen-kick": "km_big",
    "screen-kick-classic": "km_classic", "screen-kick-synth": "km_synth", "screen-perc": "km_snare",
    "screen-mixer": "x_levels", "screen-master": "r_master", "c_cdj_gen": "c_cdj_gen", "t_tracks": "t_tracks",
    "t_play": "t_play", "t_build": "t_build",
}
miss = []
for dst, src in MAP.items():
    p = out / (src + ".png")
    if p.exists():
        Image.open(p).convert("RGB").save(img / (dst + ".png"), optimize=True)
    else:
        miss.append(src)
Image.open(ldr / "ldr_50.ppm").convert("RGB").save(img / "screen-updating.png", optimize=True)
print(f"site_shots: {len(MAP) + 1 - len(miss)} pictures in {img}" + (f"; missing: {' '.join(miss)}" if miss else ""))
if miss:
    sys.exit(1)
PY
