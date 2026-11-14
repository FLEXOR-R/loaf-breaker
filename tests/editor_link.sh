#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The sound editor's link (LOAF BREAKER 0.9.45, the site's editor/ page; ui/main.c ed_sysex) end to end in the host
# simulator: ED_INFO's sections and parts; a kick part, a drum page and the kick EQ read and turned (their texts change,
# the FM-1's screen stays where it was); FX1 refused (the FX page's); the kick's wave; KICK n and SAVE.
# Needs ./build.sh --host first.
#   tests/editor_link.sh
set -e
cd "$(dirname "$0")/.."
O=app/build/editor
mkdir -p "$O"
fr() { python3 -c "import sys; sys.path.insert(0,'tools'); import cu_frames as c; print('sysex ' + c.hexs(c.frame($1, bytes([$2]))))"; }
{
echo "wait 4200"
echo "sysex_log $PWD/$O/out.syx"
echo "sysex_out"
echo "expect page 0"
fr 7 ""                                   # ED_INFO
echo "wait 50"
fr 8 "0, 1, 0, 0"                          # the kick's SUB part, read
echo "wait 50"
fr 8 "0, 1, 0, 6"                          # its LEVEL, 6 clicks right
echo "wait 50"
fr 8 "1, 0, 1, 0"                          # the snare's SOUND page, read
echo "wait 50"
fr 8 "1, 0, 1, 250"                        # its TUNE, 6 clicks left
echo "wait 50"
fr 8 "0, 11, 0, 4"                         # the kick EQ (EQ B), LOW 4 clicks right
echo "wait 50"
fr 8 "0, 13, 0, 0"                         # FX1: not the editor's
echo "wait 50"
fr 10 "244, 1"                             # the kick's wave over 500 ms
echo "wait 50"
echo "expect page 0"
fr 9 "1, 4"                                # KICK 5
echo "wait 100"
echo "expect kslot 4"
echo "expect page 0"
echo "sysex_log off"
} > "$O/editor.txt"
rm -f "$O/out.syx"
app/build/host/loaf-breaker_host "$O/editor.txt" app/build/scenarios > "$O/run.log" 2>&1 || { grep -E "FAIL|host:" "$O/run.log"; exit 1; }
python3 - "$O/out.syx" <<'P'
import sys
raw = open(sys.argv[1], "rb").read()
def unpack(p):
    u = bytearray()
    for i in range(0, len(p), 8):
        hi = p[i]
        u += bytes(p[i + j] | ((hi >> (j - 1)) & 1) << 7 for j in range(1, min(8, len(p) - i)))
    return bytes(u)
rep = []
for f in raw.split(b"\xf0")[1:]:
    f = f.split(b"\xf7")[0]
    if f[:3] == bytes([0x7D, 0x43, 0x48]) and 0x47 <= f[3] <= 0x4A:
        rep.append((f[3] & 15, f[4], unpack(f[5:])))
def knobs(d):
    return [(d[2 + 29 * i:14 + 29 * i].rstrip(b"\0").decode(), d[14 + 29 * i:30 + 29 * i].rstrip(b"\0").decode(), d[30 + 29 * i]) for i in range(4)]
info = rep[0][2]
assert rep[0][0] == 7 and info[0] == 1, "ED_INFO"
k, secs = 4, []
for s in range(info[3]):
    name = info[k:k + 8].rstrip(b"\0").decode(); n = info[k + 8]; k += 9
    parts = []
    for j in range(n):
        parts.append((info[k], info[k + 1:k + 13].rstrip(b"\0").decode(), info[k + 13])); k += 14
    secs.append((name, parts))
print("editor_link: sections " + ", ".join(f"{n} ({len(p)})" for n, p in secs))
sub0, sub1, sn0, sn1, eq, fx1, wave = rep[1:8]
a, b = knobs(sub0[2]), knobs(sub1[2])
assert a[0][0] == "LEVEL" and a[0][1] != b[0][1] and b[0][2] > a[0][2], f"kick SUB LEVEL {a[0]} -> {b[0]}"
c, d = knobs(sn0[2]), knobs(sn1[2])
assert c[1][0] == "TUNE" and c[1][1] != d[1][1], f"snare TUNE {c[1]} -> {d[1]}"
e = knobs(eq[2])
assert e[0][0] == "LOW" and e[0][1] != "0 DB", f"kick EQ LOW {e[0]}"
assert fx1[1] == 2, "FX1 refused"
assert wave[0] == 10 and len(wave[2]) == 240 and max(abs((v ^ 0x80) - 0x80) for v in wave[2]) > 20, "the kick's wave"
print(f"editor_link: kick SUB LEVEL {a[0][1]} -> {b[0][1]}, snare TUNE {c[1][1]} -> {d[1][1]}, kick EQ LOW {e[0][1]}; FX1 refused; the wave; KICK 5; the screen kept")
P
