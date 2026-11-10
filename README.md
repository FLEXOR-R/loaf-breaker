<p align="center">
<picture>
<source media="(prefers-color-scheme: light)" srcset="docs/img/banner-light.png">
<img src="docs/img/banner.png" alt="LOAF BREAKER" width="340">
</picture>
</p>

<h1 align="center">LOAF BREAKER</h1>
<p align="center"><b>A hardcore break chopper for the M-VAVE FM-1.</b><br>HARDCORE - TERROR - GABBER</p>

<p align="center">
<a href="https://flexor-r.github.io/loaf-breaker/"><b>Site</b></a> ·
<a href="https://flexor-r.github.io/loaf-breaker/emu/">Try it in the browser</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/install/">Install</a> (with your install code) ·
<a href="https://flexor-r.github.io/loaf-breaker/manual/#start-in-five-minutes"><b>Start in five minutes</b></a> ·
<a href="https://flexor-r.github.io/loaf-breaker/manual/">Manual</a> ·
<a href="../../releases">Releases</a>
</p>

LOAF BREAKER is FLEXOR's firmware for the [M-VAVE FM-1](https://www.m-vave.com/), built on
**[CHOPPAH](https://github.com/profikid/fm1-break-choppah)**, Laurens Profittlich's Amen chopper. It keeps CHOPPAH's
heart (a break cut into sixteen slices on the white keys, effect pads on the black keys, a generator, a master effects
chain, 16 patterns) and drives it into hardcore:

- **The kick first.** Its own menu, its own synth (pitch drop, hold, rumble, tune), **DISTO**, **SCREAM** and
  **DISTO +**, a **GLIDE** between notes and the rawstyle **REVERSE BASS**, its own EQ and FX (**MB KICK**, **MB OTT**),
  ten memories, a note on every step, and its real sound drawn live on screen.
- **A sequencer for live.** Kick over 16, 32 or 64 steps; per step **NOTE, HITS, COND** (chance, 1:2, 2:2, FILL, NOT
  FILL) and **NUDGE**; **ARP held = FILL**.
- **Tracks and drops.** Patterns in order with loops and per-item breaks, a radio-style PLAY, builds that don't sound
  alike (snare or kick rolls, stutter, sweeps, a gap, the **tempo rising** to the drop).
- **A mix that holds.** COMP, DUCK, MAX OUT, a HOOVER synth, rolls on the break or the kick.
- **A screen that moves.** The mix scrolling as a CDJ in rekordbox colours, the track's progress and "DROP IN 4",
  beat-lit tempo, kick flashes, screen effects (CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT), a glitched start-up.

Built on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0). No Bluetooth: MIDI over USB and the TRS jack.

![LOAF BREAKER's screens: the splash, the start screen, playing (the CDJ view), the slices, a key held, the KICK menu](docs/img/screens.png)

![Holding a pad shows its effect and puts its settings on K1-K4: roll, reverse, filter, half speed, vinyl break, random, DJ filter](docs/img/pads.png)

![The kick: its SYNTH, DIRT (DISTO), DISTO + , the classic view, the HOOVER, the kick EQ](docs/img/kick.png)

![Live: TRACKS, PLAY (the radio), a build before the drop, a step's HITS, the MASTER (COMP, DUCK, MAX OUT), SEL + knob assignments](docs/img/live.png)

- **The manual**: [online](https://flexor-r.github.io/loaf-breaker/manual/), or [app/docs/MANUAL.md](app/docs/MANUAL.md)
- **Download the firmware**: [releases](../../releases) (`loaf-breaker-X.YY.fwsc`), each release also carries the
  site (emulator, installer, manual) as `loaf-breaker-site-X.YY.zip`. On the site the firmware is encrypted: the
  installer asks for the **install code** (the repository's Actions secret `LOAF_INSTALL_CODE`; without it, installing
  is closed; `tools/site_lock.py` does it).
- **Install**: fm1-base's web installer (Chrome or Edge, the FM-1 plugged in), or
  `python3 base/tools/fm1_install.py loaf-breaker-X.YY.fwsc`

Installing is at your own risk; the installer can put M-VAVE's own firmware back, and an FM-1 that fails to start
twice comes up in a safe mode the installer can still reach. If your FM-1 runs M-VAVE's FM1 Open firmware, restore
M-VAVE's original firmware first ([docs/FLASHING.md](docs/FLASHING.md)).

**Status: 0.1xx, moving fast.** Every push to the `loaf` branch is a release. LOAF BREAKER has no Bluetooth.

## Panel

| Control | Does |
| --- | --- |
| White keys 1–16 | Slices: at the next snap point while playing, at once while stopped. **Hold to loop**: K1–K4 set that key's loop (length, pitch, start, mode). **Key held + FX**: that slice's own effect |
| Black keys | Pads, while held: **R8 · R16 · R32** rolls (K2 THRU: the break, the kick or both; SEL + a roll pad: a kick roll for that press) · **REV** reverse · **RND** random · **HLF** half speed (with a LATCH) · **VNL** vinyl break · **ARP** an overlay effect · **KIL** the sequencer's kicks quiet while held (or latched) · **TER** the kick's DISTO off / on · **SUB** SUB DRIVE (the gate: HOME's K1, the DJ filter: K2). K1–K4 tune the pad held |
| PLAY | Play / stop · HOME + PLAY: back to bar 1 |
| SELECT | Tempo · **SEL + SELECT**: the drums' filter (DRUM FLT) on any page |
| SEL | **Tapped alone**: the shortcuts card, those that work everywhere and this page's own · **SEL ×2**: locked down, lit, for the combinations one-handed (press again: free) · **SEL + K1–K4**: four live functions you assign on EDIT › KNOBS (or all four at once with a STYLE: TERROR, EARLY NEXT GEN, HARDCORE …), each kick its own (PITCH DROP, RUMBLE, PUNCH, DRIVE, DISTO, SCREAM, DISTO +, its TYPE, SCREECH, TAIL…) · **SEL + REC** (on FX): the effect under your hands back to its start (again within 4 s: undo) · **SEL + ENV**: the drop · **SEL + ALGORITHM** the break, **SEL + PRESETS** the pattern · **SEL + SEQ** copy, then paste the pattern · **SEL + SAVE** undo · SEL alone: every combination on the strip |
| HOME | PERFORM and GENERATOR, with the **CDJ view** (the mix scrolling in, rekordbox's RGB colours, the playhead at the right edge); HOME held: the slices |
| EDIT | The **KICK menu**: SOUND, SYNTH (DROP, HOLD, RUMBLE, ASYM, TUNE…; BASS: GLIDE between notes, the rawstyle REVERSE BASS), DIRT (DISTO, SCREAM, DISTO +), TONE (EQ and two FX slots of its own), KNOBS (what SEL + K1–K4 turn, each kick its own, their settings and functions); EDIT again: the next section; EDIT held: the classic view. **SEL + EDIT**: the snare, the hat, the clap and the HOOVER |
| SEQ | The sequencer: the break's steps and the drum lanes, the kick over 16, 32 or 64 steps. **A step held**: K1 NOTE, K2 HITS (1–4 taps in the step), K3 COND (chance, 1:2, 2:2, FILL, NOT FILL), K4 NUDGE. Playing, the keys follow the bar playing. **SEQ held** + key 1–4: copy the bar there; + K1: shift the lane a step; + REC: clear the drums; then SEQ again: clear the pattern. **REC** on a drum lane: tap it in |
| ENV | **TRACK**: TRACKS (patterns, loops, drops and each item's own break and kick; ENV held: a space after the item, or the space taken out), PLAY (a radio: tune a track, press PLAY), DROP, BUILD (the build before a drop: style, sweep, riser, a gap; PRESETS: the tempo rising to the drop) |
| LFO | **LFO held + K1–K4**: the break, kick, drums and FX levels on any page · The **MIXER**: LEVELS, GROOVE, MASTER (COMP, RATIO, DUCK, MAX OUT), DYN (the compressor's and the ducker's timing) |
| FX | The master FX chain: four slots (FILTER, CRUNCH, DRIVE, PUNCH, FLANGER, RING MOD, TREMOLO, REVERB, DELAY, EQ, TAPE, MB COMP, CLIP); each effect remembers its settings when you change it and back; presets MB KICK and MB OTT after MB COMP |
| ARP | **Held: FILL** (the steps set FILL play, NOT FILL rest, the snare rolls the bar's last beat) · SEL + ARP: a random pattern |
| GLO | GLOBAL: MIDI clock in and out (USB, TRS); the pitch and snap, RESET ALL; MORE: MODE (BEGINNER: the KICK menu's basics; EXPERT: all of it), AUTO SAVE, 18 skins, WAVE ZOOM; EFFECTS: the screen effects, several at once (CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT); FX SET: their settings; RESET: one part at a time |
| SAVE | Keep the settings, the patterns, the tracks and the steps |

## Breaks

LOAF BREAKER has 9 breaks on ALGORITHM, 2 bars each: AMEN10 135, NUSKOOL 140, AMEN20 164, AMEN19 172,
UPDOWN 165, AMEN15 174, THINK 114, APACHE 115 and THEWORM 110. [`breaks/breaks.txt`](breaks/breaks.txt) marks with `*` the ones
built in (the bank has 400 KB); `./build.sh` builds them into `app/bank_data.h` (generated, git-ignored). The breaks
are built in: LOAF BREAKER does not load samples of your own.

## Building

```
scripts/bootstrap.sh    # once: fm1-base (submodule), JieLi toolchain, AC79 SDK files, Python packages
./build.sh test         # the engine's tests, plain cc, no toolchain needed
./build.sh              # app/build/loaf-breaker.fwsc (the build's files keep CHOPPAH's name) (macOS: the toolchain runs in Docker / OrbStack)
./build.sh --host       # the host simulator: app/build/host/loaf-breaker_host tests/scenarios/ui.txt OUTDIR
./build.sh --web        # the browser emulator (Emscripten)
./build.sh flash        # install over USB-MIDI (docs/FLASHING.md)
```

Clone with `--recursive` (fm1-base is a submodule in `base/`).

| Path | What |
| --- | --- |
| `app/` | The firmware: `app.c` (screens, controls, glue), `app.h`, `fm1.toml` (build and site settings) |
| `app/dsp/` | The engine, platform-free: `brk` (sequencer, generator, pads, voices), `brk_fx`, `adpcm`, `brk_synth`, `brk_bank.h` |
| `app/docs/`, `app/site/` | The manual and the site's front page |
| `tests/engine/` | Engine tests (plain cc), with WAV renders |
| `tests/scenarios/` | Host-simulator scripts: keys in, audio and screenshots out |
| `breaks/` | The recorded breaks (WAV) and `breaks.txt`, which of them go into the firmware |
| `tools/prep_breaks.py` | WAV → slices → IMA ADPCM bank (`--list breaks/breaks.txt`, as `./build.sh` runs it) |
| `docs/` | [PRD](docs/PRD.md), [flashing and recovery](docs/FLASHING.md), [the FM-1 Firmware Hub's submission](docs/hub/SUBMIT.md) |

## Publishing

Every push to the `loaf` branch is a release (`.github/workflows/loaf.yml`): the engine tests, the firmware and
its size report, the host scenarios, then a GitHub release `loaf-vX.YY` with `loaf-breaker-X.YY.fwsc` and the site
built from the same commit (front page, installer, emulator, manual with its screenshots: `loaf-breaker-site-X.YY.zip`).
Past 0.99 the numbers go on, 0.100, 0.101…; the package itself says 0.99.N for fm1-base.

The manual's and the site's screenshots come from the host simulator: `./build.sh --host`, then
`sh tools/site_shots.sh` (into `app/docs/img`).

## Licence and credits

MPL-2.0 ([LICENSE](LICENSE)). LOAF BREAKER is built on [CHOPPAH](https://github.com/profikid/fm1-break-choppah) by
Laurens Profittlich, itself on [fm1-base](https://github.com/charlesvestal/fm1-base) and
[felucca-bsp](https://github.com/hugelton/felucca-bsp), from [Felucca](https://github.com/hugelton/Felucca) by Leo
Kuroshita (Hügelton Instruments).

M-VAVE and FM-1 are trademarks of their owners. LOAF BREAKER is not affiliated with M-VAVE, nor with CHOPPAH's author.
