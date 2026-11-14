# flexor/: FLEXOR's breaks

Breaks FLEXOR found while trying the BREAKS page (0.9.32), sampled from artists' records: they are **not** royalty
free, their rights belong to the artists and labels. They are here because FLEXOR chose to publish them; anyone building
on this should make their own call.

Put the WAVs here (any sample rate and bit depth; 16-bit or 24-bit PCM, or 32-bit float), whole bars, then give each
a line in [`../breaks.txt`](../breaks.txt): `flexor/FILE.wav`, its BPM, the bars that go into the firmware (2), the bars
the WAV holds, a name of up to 11 characters. A `*` puts it in the firmware; every line is in the site's library.

| File | Artist, track | BPM | Bars | In the firmware |
| --- | --- | --- | --- | --- |
| `akira_custom_grind.wav` | Akira, "Custom Grind" (drop break) | 190 | 2 | AKIRA 190 (0.9.34) |
| `detest_tripped_hardcore_to_the_penis.wav` | Detest & Tripped, "Hardcore To The Penis" (intro break) | 175 | 2 | DETEST 175 (0.9.34) |
| `lil_texas_louder.wav` | Lil Texas, "LOUDER" (break) | 175 | 2 | LOUDER 175 (0.9.34) |
| `the_satan_gangzta_cash.wav` | The Satan, "Gangzta Cash" (intro break) | 200 | 2 | SATAN 200 (0.9.34) |
| `tripped_matt_green_underground_panther_jacket.wav` | Tripped & Matt Green, "Underground Panther Jacket" (break) | 170 | 2 | PANTHER 170 (0.9.34) |
| `artheist_the_obedience_algorithm.wav` | Artheist, "The Obedience Algorithm" (break) | 165 | 2 | OBEDIENC165 in 0.9.37; in the library since 0.9.38 |
| `dolphin_rotterdam_shuffle.wav` | Dolphin, "Rotterdam Shuffle" (intro break) | 190 | 2 | SHUFFLE 190 (0.9.38, in OBEDIENC165's place) |
FLEXOR's breaks are sliced on an even grid (FLEXOR: "EVEN GRID", as he sends them from the BREAKS page): `*=` (or `-=`)
in breaks.txt; and louder than the others: -12 dBFS RMS, 4 dB above the other breaks' -16 (0.9.35 had them at their files' own
level, FLEXOR: too loud). **Every break put here gets that level** (FLEXOR, 14 Nov 2026: his DJ library, mastered
alike; tools/prep_breaks.py `FOLDER_LOUD`), unless its line in breaks.txt says another (`@X` after the name). Their first three (Akira, "Custom Grind"; Detest & Tripped, "Hardcore to The Penis"; The Satan, "Gangzta
Cash") were taken out on 14 Nov 2026 for FLEXOR to make them again.
