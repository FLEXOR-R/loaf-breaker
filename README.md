# LOAF BREAKER: a hardcore break chopper for the M-VAVE FM-1

LOAF BREAKER is FLEXOR's firmware for the [M-VAVE FM-1](https://www.m-vave.com/), built on
**[CHOPPAH](https://github.com/profikid/fm1-break-choppah)**, Laurens Profittlich's Amen chopper. It keeps CHOPPAH's
heart (a break cut into sixteen slices on the white keys, effect pads on the black keys, a generator, a master
effects chain, 16 patterns) and pushes it toward terror, gabber and crossbreed: a kick with its own menu, its own
synth, TERROR, TERROR 2 and TERROR +, its own effects and a note on every step; a sequencer of 16, 32 or 64 steps with
hits, chance and nudge per step; tracks of patterns with built-up drops; a HOOVER; a compressor, a ducker and an
anti-clip MAX OUT; and a CDJ-style RGB waveform of what leaves the mix. Built on
[fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0).

![LOAF BREAKER's screens: the splash, the start screen, playing (the CDJ view), the slices, a key held, the KICK menu](docs/img/screens.png)

![Holding a pad shows its effect and puts its settings on K1-K4: roll, reverse, filter, half speed, vinyl break, gate, random, DJ filter](docs/img/pads.png)

![The kick: its SYNTH, DIRT (TERROR), TERROR + , the classic view, the HOOVER, the kick EQ](docs/img/kick.png)

![Live: TRACKS, PLAY (the radio), a build before the drop, a step's HITS, the MASTER (COMP, DUCK, MAX OUT), SEL + knob assignments](docs/img/live.png)

- **The manual**: [app/docs/MANUAL.md](app/docs/MANUAL.md)
- **Download the firmware**: [releases](../../releases) (`loaf-breaker-X.YY.fwsc`), each release also carries the
  site (emulator, installer, manual) as `loaf-breaker-site-X.YY.zip`. On the site the firmware is encrypted: the
  installer asks for the **install code** (the repository's Actions secret `LOAF_INSTALL_CODE`; without it, installing
  is closed). `tools/site_lock.py` does it
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
| Black keys | Pads, while held: **R8 · R16 · R32** rolls (K2 THRU: the break, the kick or both; SEL + a roll pad: a kick roll for that press) · **REV** reverse · **FLT** filter throw · **HLF** half speed (with a LATCH) · **VNL** vinyl break · **GAT** gate · **RND** random · **ARP** an overlay effect · **SUB** SUB DRIVE. K1–K4 tune the pad held |
| PLAY | Play / stop · HOME + PLAY: back to bar 1 |
| SELECT | Tempo (SEL held: 0.1 BPM) · SEL + SELECT: the kick's memories 1–10 |
| SEL | **SEL + K1–K4**: four live functions you assign (DRIVE, TERROR, TERROR 2, TERROR +, its TYPE, SCREECH, TAIL…) · **SEL ×2**: the effect under your hands back to its start (again within 4 s: undo) · **SEL ×3**: the drop · **SEL + ALGORITHM** the break, **SEL + PRESETS** the pattern · **SEL + SEQ** copy, then paste the pattern · **SEL + SAVE** undo · SEL alone: every combination on the strip |
| HOME | PERFORM and GENERATOR, with the **CDJ view** (the mix scrolling in, rekordbox's RGB colours, the playhead at the right edge); HOME held: the slices |
| EDIT | The **KICK menu**: SOUND, SYNTH (DROP, HOLD, RUMBLE, ASYM, TUNE…), DIRT (TERROR, TERROR 2, TERROR +), TONE (EQ and two FX slots of its own); EDIT again: the next section; EDIT held: the classic view. **SEL + EDIT**: the snare, the hat, the clap and the HOOVER |
| SEQ | The sequencer: the break's steps and the drum lanes, the kick over 16, 32 or 64 steps. **A step held**: K1 NOTE, K2 HITS (1–4 taps in the step), K3 CHANCE, K4 NUDGE. SEQ held, then SEQ again: clear the pattern |
| ENV | **TRACK**: TRACKS (patterns, loops, drops and each item's own break; ENV held: a space after the item, or the space taken out), PLAY (a radio: tune a track, press PLAY), DROP, BUILD (the build before a drop: style, sweep, riser, a gap), KNOB (what SEL + each knob does), RESET |
| LFO | The **MIXER**: LEVELS, GROOVE, MASTER (COMP, RATIO, DUCK, MAX OUT), DYN (the compressor's and the ducker's timing) |
| FX | The master FX chain: four slots (FILTER, CRUNCH, DRIVE, PUNCH, FLANGER, RING MOD, TREMOLO, REVERB, DELAY, EQ, TAPE, MB COMP, CLIP); each effect remembers its settings when you change it and back |
| ARP | OVERLAY: sixteen effects on the white keys over everything (DIVE, RISE, CRUSH, REESE…) · SEL + ARP: a random pattern |
| GLO | GLOBAL: MIDI clock in and out (USB, TRS); the pitch and snap, RESET ALL; MORE: the skins, AUTO SAVE |
| SAVE | Keep the settings, the patterns, the tracks and the steps |

## Breaks

LOAF BREAKER has 9 breaks on ALGORITHM, 2 bars each: THINK 114, NUSKOOL 140, UPDOWN 165, APACHE 115, CHOPPER 172,
DRUMMER 100, THEWORM 110, HOTPANT 115 and AMEN19 172. [`breaks/breaks.txt`](breaks/breaks.txt) marks with `*` the ones
built in (the bank has 400 KB); `./build.sh` builds them into `app/bank_data.h` (generated, git-ignored). The breaks
are built in: LOAF BREAKER does not load samples of your own.

## Building

```
scripts/bootstrap.sh    # once: fm1-base (submodule), JieLi toolchain, AC79 SDK files, Python packages
./build.sh test         # the engine's tests, plain cc, no toolchain needed
./build.sh              # app/build/choppah.fwsc (the build's files keep CHOPPAH's name) (macOS: the toolchain runs in Docker / OrbStack)
./build.sh --host       # the host simulator: app/build/host/choppah_host tests/scenarios/ui.txt OUTDIR
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
