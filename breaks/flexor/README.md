# flexor/: FLEXOR's breaks

Breaks FLEXOR found while trying the BREAKS page (0.9.32), sampled from artists' records: they are **not** royalty
free, their rights belong to the artists and labels. They are here because FLEXOR chose to publish them; anyone building
on this should make their own call.

Put the WAVs here (any sample rate and bit depth; 16-bit or 24-bit PCM, or 32-bit float), whole bars, then give each
a line in [`../breaks.txt`](../breaks.txt): `flexor/FILE.wav`, its BPM, the bars that go into the firmware (2), the bars
the WAV holds, a name of up to 11 characters. A `*` puts it in the firmware; every line is in the site's library.

| File | Artist, track | BPM | Bars | In the firmware |
| --- | --- | --- | --- | --- |
| `akira_custom_grind.wav` | Akira, "Custom Grind" (drop break) | 190 | 4 | AKIRA 190, the first (0.9.34) |
| `detest_tripped_hardcore_to_the_penis.wav` | Detest & Tripped, "Hardcore to The Penis" (intro break) | 175 | 4 | DETEST 175 (0.9.34) |
| `the_satan_gangzta_cash.wav` | The Satan, "Gangzta Cash" (intro break) | 200 | 4 | SATAN 200 (0.9.34) |

The tempos come from their lengths (whole bars, 16-bit 44.1 kHz stereo WAV); the firmware takes their first two bars.
