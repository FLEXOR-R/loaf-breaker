# CHOPPAH — Amen Chopper firmware for the M-VAVE FM-1

> **LOAF BREAKER** (FLEXOR's fork, 2026) builds on this PRD: CHOPPAH's sections below are where it started, and each
> LOAF BREAKER change has its own row (marked "LOAF BREAKER", A-50 onwards). It has no Bluetooth, kicks and drums
> only (no melody), 9 breaks. Its 1.0 (§13, M8) waits on launch points, not code: the repository made public, the
> tests on an FM-1 (installs from M-VAVE's V15 and from another firmware, another firmware back over it, an hour's
> play), the breaks' rights, and the site's install code (pointless once public).

**Product requirements document** · v0.1 · 30 Oct 2026 · Owner: Laurens (Sylverline)
Status: draft for review · Working name: CHOPPAH (repo `fm1-break-choppah`) · Repo docs: `docs/PRD.md`

---

## 1. Summary

CHOPPAH turns the M-VAVE FM-1 into a dedicated breakbeat chopper: load the Amen (or any break), cut it
into slices, and play, sequence, randomise and mangle it live with remix effects and DSP. It has to match
everything the **Amen Break Generator** app does (§4), then go further with a **Performance mode** (§5)
built for jungle / drum & bass / breakcore sets: momentary FX on the black keys, rolls and stutters,
tape stops, scratch scrub, per-step locks and pattern chaining.

The firmware is built on **fm1-base** (MPL-2.0) and **felucca-bsp**: the same platform as AMB-1, LOOM-8
and CTL-1. It has a host simulator and a browser emulator, and installs through the USB update path with
a safe mode and a route back to stock firmware.

### 1.1 Goals

1. **Parity:** every feature of Amen Break Generator (Revived), mapped onto the FM-1 panel.
2. **Performance:** a hands-on chop-and-FX mode that's playable live without looking at the screen.
3. **Sound:** a crunchy, 90s-sampler character (repitch, not time-stretch, by default) with clean
   headroom: no clicks at slice edges and no dropouts.
4. **The breaks:** a desktop tool that slices, levels and encodes the breaks built into the firmware
   (`breaks/breaks.txt`). Loading samples of your own onto the device is out of scope (3 Nov 2026, §4.7).
5. **Safe to install:** never brick the device; always a way back to stock.

### 1.2 Non-goals (v1)

- A general-purpose sampler or multitrack groovebox (X0X, LOOM-8 and zp12 already do that).
- Synth engines (FM, 303, reese). A bass layer is listed under Future (§12).
- Wi-Fi. (Bluetooth MIDI came in 30 Oct 2026, A-71: its ~40 KB of flash cost one recorded break.)
- Recording audio from a microphone. The FM-1 has no audio input.

### 1.3 Target users

- **Laurens as DJ / live performer:** jungle and DnB sets, B2B with CDJs, needs MIDI clock sync.
- **Beat-makers:** want a pocket Amen chopper to sketch patterns and record them into a DAW over USB.
- **The FM-1 community:** installs from a web page; reads the manual.

---

## 2. Hardware and platform constraints

| Item | Value | Source / implication |
| --- | --- | --- |
| SoC | JieLi AC791N (WL82), pi32v2 core, single-precision FPU, **second core available** | fm1-base `plat_cpu2_*`; FX chain can move to core 2 |
| Audio | 44.1 kHz, stereo, 24-bit render, 256-frame blocks (5.8 ms) | `app_render(lr, n, master_q12)` |
| App flash slot | 581 KB | code + built-in assets |
| RAM | 96 KB `.data`/`.bss` + **336 KB pool** for big buffers | PCM decode buffers live in the pool (§8.2) |
| Sample flash | Data area 0x97000–0xDFFFF (292 KB, shared with store objects), XDATA0 16 KB, XDATA1 92 KB → **≈ 356 KB for the bank** (0x97000–0xD9FFF + XDATA1, §8.2) | Sector bytes 0xF00–0xF07 must stay erased in XDATA windows |
| Screen | 240 × 240 colour TFT, ~60 fps UI frame | |
| Keys | 27 silicone keys F3–G5: **16 white, 11 black**, each with an LED; velocity and pressure via `APP_TOUCH` (experimental) | 16 white = 16 slices/steps; 11 black = FX pads |
| Buttons (14) | FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY (red/green), REC, OCT−, OCT+ | |
| Encoders (7) | SELECT, ALGORITHM, PRESETS, K1–K4 (detented, endless) | |
| Pot | MASTER volume | |
| MIDI | USB-MIDI in/out, TRS MIDI **in only**, Bluetooth MIDI in/out | clock out over USB and Bluetooth |
| USB audio | FM-1 as a USB audio input (the master output, recordable in any DAW) | covers "record to WAV" (§4.6) |
| Store | Torn-write-safe objects of ≤ 3840 bytes each (A/B sectors) | patterns split across objects |
| Update | USB-MIDI SysEx OTA; single bank; fm1-base safe mode after two failed boots; stock restore from the web installer | §10 |

---

## 3. Panel map (overview)

The panel works in two layers. **Keys** are always the instrument; **buttons** pick what K1–K4 edit.

| Control | Default role |
| --- | --- |
| **White keys 1–16** | Slices 1–16 (live trigger, quantised to the step grid while playing; immediate while stopped); in SEQ, the 16 steps |
| **Black keys 1–11** | Performance FX pads (momentary; §5.1) |
| **PLAY** | Play / pause (green playing, red when the looper records) |
| **HOME** | Main screen; **hold HOME + PLAY** = reset to bar 1; **HOME + SELECT** = tempo |
| **REC** | Looper: record → loop → overdub → loop (§4.3); hold = clear |
| **SEQ** | Step sequencer edit |
| **ARP** | OVERLAY: sixteen effects on the white keys (B-52); SEL + ARP: randomize (the generator is HOME's second part since 3 Nov 2026) |
| **EDIT** | Break / slice editor |
| **FX** | DSP effects page (§4.5) |
| **ENV** | Slice envelope and gate |
| **LFO** | Modulation (filter / flanger / ring-mod rate, tempo-synced) |
| **GLO** | Global: MIDI, clock, swing, master bus, settings |
| **SEL** | Shift: second page of the current screen; with a key, alternative functions |
| **OCT− / OCT+** | Pitch/speed down / up (hold to accelerate); both = reset to 0 |
| **SAVE** | Save pattern and kit; hold = save as / revert |
| **SELECT enc** | Scroll / value; with HOME held, tempo |
| **ALGORITHM enc** | Break select (1–8 slots + built-ins) |
| **PRESETS enc** | Pattern select (queued to the next bar) |
| **K1–K4** | Parameters of the current page (labels on screen) |
| **MASTER** | Volume |

**Hold any button for 1 s** for a help card (fm1-base pattern), so the panel explains itself.

---

## 4. Feature set A: Amen Break Generator parity

Each requirement has an ID, priority (**P0** must ship in v1.0, **P1** should, **P2** nice to have) and
acceptance criteria.

### 4.1 Break playback and slice triggering

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-1 | Play a break as a tempo-locked loop. Default repitch mode: playback rate = target BPM / break's native BPM | P0 | Amen at 136.9 native plays in time at 60–220 BPM; ±1 sample drift per 64 bars max |
| A-2 | Break library: the built-in breaks (8, `breaks/breaks.txt`) | P0 | ALGORITHM switches breaks, quantised to the next bar. *Done 3 Nov 2026: the name shows at once (amber in the header, "NEXT BREAK"); in the bar's last 16th the break is muted (the clock, the drums and the steps run on), the next one decoded over the same PCM buffer, and the engine swaps it in on the next step, the bar's first (`brk_mute`, `brk_swap_break`); stopped, at once. Engine test test_swap; scenario breaks.txt* |
| A-3 | **16 slice trigger keys** (white keys), synced to tempo: a press plays that slice at the next step (1/16 grid) | P0 | Max quantise latency = 1 step; trigger jitter < 1 ms |
| A-4 | **Free-play mode:** while stopped, keys play slices immediately (unquantised) | P0 | Key-to-sound < 10 ms |
| A-5 | Slice count per break: 8, 16 (default) or 32; equal grid or transient-detected markers from the prep tool | P0 | Markers survive reboot |
| A-6 | Click-free slice edges: 1–3 ms fades at slice start/stop and on every cut | P0 | No audible clicks on rapid retriggers at 1/32 |
| A-7 | Reload the last break, pattern, tempo and settings at power-on | P0 | State identical after power cycle |
| A-8 | Velocity from keys scales slice volume (`APP_TOUCH`) | P2 | Off by default; GLO toggle |

### 4.2 Randomising algorithm (generator)

Based on the behaviour of BB Gen / X0X's break part and ABG's "randomizing algorithm".

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-10 | **Generator** (HOME's second part, HOME again; the ARP page until 3 Nov 2026) rearranges slices as the loop plays. Parameters: **COMPLEXITY** (chance of jumping instead of playing the next slice), **ANCHOR** (keep kick/snare slices on 1 and 3), **ROLL** (repeat or neighbour-slice chance), **FILL** (rule-breaking in the last bar of a phrase) | P0 | COMPLEXITY 0 = straight loop, bit-exact with plain playback |
| A-11 | **RANDOMIZE** one-shot: SEL+ARP writes a new random pattern into the current sequence | P0 | Undo with a second SEL+ARP within 4 s |
| A-12 | Phrase length 2/4/8/16 bars (fills land on the phrase's last bar) | P1 | |
| A-13 | Deterministic seed per pattern (same seed → same variations), and a "lock" so a pattern you like can be recalled | P1 | |
| A-14 | Retrigger chances per bar: 2×, 3×, 4×, 8× | P1 | |

### 4.3 Looper

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-20 | **Looper repeats the last 16 steps** of what was played (keys, generator and FX pad moves, as events, not audio) | P0 | REC tap: the next 16 steps loop exactly |
| A-21 | **Overdub:** REC again adds new key presses on top of the loop | P0 | |
| A-22 | Loop length 4, 8, 16, 32 or 64 steps (GLO) | P1 | |
| A-23 | Commit loop to the current pattern (SAVE while looping) | P1 | Pattern replays the loop identically |
| A-24 | Undo last overdub layer | P2 | |

### 4.4 Live remix effects (transport-locked)

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-30 | **FREEZE:** hold the current slice (or 1/8, 1/16 loop of it) while transport keeps counting; release rejoins where the loop would be | P0 | Rejoin lands on the correct step |
| A-31 | **Step randomisation:** each step plays a random slice while held | P0 | |
| A-32 | **Auto-retrigger:** repeat the current slice at a rate (1/8, 1/16, 1/32, triplets) while held | P0 | |
| A-33 | **Slice re-trigger:** re-fire the slice playing now from its start | P0 | |
| A-34 | **RMX mode:** generator on/off as a latch; the playhead follows the loop's own position, so dropping out returns in phase | P0 | |
| A-35 | **REVERSE as a gate:** reverse playback while held (not a toggle) | P0 | |
| A-36 | **Pitch / speed ±** on OCT−/OCT+: semitone steps; holding accelerates; both buttons reset | P0 | ±24 semitones; fine mode with SEL (±100 cents) |
| A-37 | **Pause** (PLAY) and **reset** (HOME+PLAY) | P0 | |
| A-38 | **Tap tempo** (2–3 Nov 2026 on ENV with a metronome click). LOAF BREAKER: *removed* (the tempo is SELECT's); ENV is now **TRACK** (A-38b) | — | *Removed* |
| A-38b | **TRACK** (LOAF BREAKER, ENV; ENV again: the next part, as HOME's). **TRACKS**: 8 tracks (PRESETS), each up to 32 items in four rows of eight: a pattern (P1–P16) played for its loops (×1–×16), a **DROP** (the build, then the drop into the pattern after it), the END; K1 the item, K2 what it is (END: deleted, the rest moves up), K3 its loops, K4 what the track does at its end (LOOP, STOP, HOLD); a white key puts its pattern at the item and moves on. **PLAY**: K1 the track, K2 the item it starts from; PLAY (or white keys 1–8) plays it from bar 1 and goes HOME (the header shows T1); a pattern comes in at the end of its last loop, a DROP's build starts at the next bar; stopping ends it; SEL + ENV while a track plays drops into its next pattern. **DROP**: K1 the build (none, 1, 2, 4, 8 bars; none: the bar playing ends in the drop), K2 the pattern after a SEL + ENV drop (NEXT, P1–P16), K3 the drop's break, K4 the crash's level. SEL + ENV now starts the build at the next bar. Tracks are saved after the patterns (an older pattern save loads) | P1 | *Done* |
| A-49c | **Effects remembered, and their reset** (LOAF BREAKER): an FX slot turned to another effect keeps the one it left as it was (P1-P3, MIX, MORE, MOD; one memory per effect, saved with the patterns), and an effect turned back to comes back so; the same for each kick layer's effect (each layer, each effect). SEL ×2 on FX: the focused slot's effect to its starting settings (MB COMP's detail too) and forgotten; on EDIT KICK's FX parts: that layer's effect; SEL ×2 again within 4 s undoes it | P1 | *Done; scenario fxmem.txt* |
| A-49d | **KICK FX** (LOAF BREAKER, HOME's fourth part after KICK EQ): two FX slots on the kick alone (after its EQ, before the THRU / chain routing), the FX library but REVERB and DELAY (one of each in RAM: the chain's); the FX page's views, parts, MOD, MB COMP detail, SEL + FX bypass, FX + ALGO move, SEL ×2 reset; their own effect memory. Engine slots 4, 5 (brk_fx_kick: the same per-sample loop, a mode); shed first when the render runs over | P1 | *Done; engine test, scenario kfx.txt* |
| A-49e | **COMP and KNOB** (LOAF BREAKER): LIMIT (SEL + K3) replaced by a master **COMP** (its amount the threshold 0 … −36 dB, ratio 1.5–20:1, attack 0.1–30 ms, release 20–800 ms, half the reduction made up, held under full scale; its reduction metered). ENV's fourth part **KNOB**: each SEL + knob function's settings (ALGORITHM / PRESETS / keys 1–4 pick it; K1 its amount): DUCK attack, release and what it ducks (the break, or the drums too); TERROR 2 SUB, SCREAM, PUSH; COMP; TERROR SUB, FIZZ, SUSTAIN (the TERRORs' in the kick's spare bytes: with its memory; 0 as before) | P1 | *Done; scenario knob.txt* |
| A-49f | **CDJ view** (LOAF BREAKER): PERFORM and GENERATOR's picture (y 88–191, the default): the mix as it leaves (before MASTER) scrolling right to left, the playhead at the right edge; per column (44100 / BPM samples: 15 a step, 4 beats on screen) its lows (< ~200 Hz), mids, highs (> ~2.5 kHz) as rekordbox RGB and its peak as height; beat ticks, the bar's first lit, BAR.BEAT. The render fills a 240-column ring in the pool (scope, ~1 KB); ~30 frames a second. HOME held 0.35 s: the slice blocks instead, held again: back (HOME's part unchanged) | P2 | *Done; scenario cdj.txt* |
| A-49g | **No saturation** (LOAF BREAKER): a last guard after COMP (a peak over 0.95 brought down at once, back over ~50 ms; COMP's hard clamp gone); GLOBAL MORE K4 **MAX OUT** (in DROP BRK's place, which is ENV DROP's K3): the output's most at MASTER full, 0 … −12 dB, −3 dB the default | P1 | *Done; scenario maxout.txt (MASTER full, COMP at its most: under −2.5 dBFS; MAX OUT −6: under −5.5)* |
| A-49h | **Menus by use** (LOAF BREAKER, 8 Nov 2026, tested as TEST REORG): HOME plays (PERFORM, GENERATOR); EDIT KICK has it all (… OUT, EQ BANDS, EQ SHAPE, FX 1, FX 2, TERROR, TERROR + (DRIVE, TYPE FOLD/CLIP/CRUSH, SCREECH, TAIL: the kick layers' spare bytes), TERROR 2); MIXER: LEVELS, GROOVE, MASTER (COMP, RATIO, DUCK, MAX OUT); GLOBAL MORE: SKIN, AUTO SAVE. ENV's KNOB: each of SEL + K1..K4 given a function (DUCK, TERROR 2, COMP, TERROR, T+ DRIVE / TYPE / SCREECH / TAIL, DRIVE = INTENSITY; selk_asg, selk_asg2) | P1 | *Done; scenario reorg.txt, engine test TERROR +* |
| A-49i | **Drops unlike each other** (ENV's BUILD): STYLE (SNARE ROLL, KICK ROLL, KICK+SNARE, STUTTER: the break rolled by the roll pads 8ths → 16ths → 32nds, SWEEP ONLY: the drums on, RANDOM), SWEEP (HP, LP ~18 kHz → 400 Hz, none, RANDOM), RISER (0–200 %), GAP (silence before the drop: 0, 1, 2, 4, 8 steps, ~1 ms in and out) | P1 | *Done; scenario drops.txt (a beat's silence before each drop in its sound)* |
| A-49j | **RESET** (ENV's last part): K1 a list of parts (DROP, BUILD, KNOB to start with), K4 right twice within 3 s puts that part's settings back as they start (DROP: 4 bars, NEXT, SAME, 100 %; BUILD: SNARE ROLL, HP, 100 %, no gap; KNOB: the knobs' own functions, DUCK and COMP off with their first settings). ENV's tabs drop the button's name for room | P2 | *Done; scenario reset.txt* |
| A-49k | **Kick SYNTH** (EDIT KICK's last parts, ALGORITHM left from MIX): SYNTH (DROP: the SUB's pitch drop x0.25..x4; TIME 30–480 ms; SHAPE: the SUB clean .. squared; HOLD: to 250 ms at full, or a faster decay) and SYNTH + (RUMBLE: the kick < ~150 Hz into a feedback line (pool, 4096 samples) low-passed again; LENGTH 23–93 ms with its feedback; ASYM: an uneven drive before the clip, its offset taken off; TUNE ±12 semitones). Settings' ksyn and ksmem (per kick memory); 0: as before. ENV's PLAY drawn as a radio (a dial, a gliding needle, a tuning knob) | P1 | *Done; engine test KICK SYNTH, scenario synth.txt* |
| A-49l | **HOOVER** in the RAVE STAB's place (LOAF BREAKER, the fifth drum, its lanes V1/V2, its notes per step as before, MIDI 40): six saws spread around the note (two an octave down, half of them set against another: sweeping pulses), starting BEND octaves below (up to 2.5) and rising into the note, two low-passes (TONE), DRIVE; root 55–220 Hz, length 80–1480 ms; presets CLASSIC, DIVE, SCREAM, DARK | P1 | *Done* |
| A-49m | **The KICK menu** (EDIT): four sections, EDIT the next, ALGORITHM the next part in it (remembered): SOUND (MIX, SUB, PUNCH, GRIT, CLICK, OUT), SYNTH (SYNTH, SYNTH +, MACRO), DIRT (TERROR, TERROR +, TERROR 2, the layers' FX), TONE (EQ BANDS, EQ SHAPE, FX 1, FX 2). Its picture (y 44–143, no break waveform): the hit's wave over 250 ms (kick_wave: each column's sample furthest from 0) over its level, the SUB's pitch fall (amber, log 30 Hz–1 kHz), KICK n, its note, the layers' levels; the part's knobs as cards (y 144–191). SEL + EDIT: the other drums | P1 | *Done; scenario kickmenu.txt* |
| A-49n | **DUCK and COMP leave KNOB** (the MIXER has them: MASTER, and a fourth part DYNAMICS: COMP's attack and release, DUCK's release and what it ducks): a SEL + knob may be TERROR 2, TERROR, T+ DRIVE / TYPE / SCREECH / TAIL or DRIVE; its own (code 0): K1 DRIVE, K2 TERROR 2, K3 T+ SCREECH, K4 TERROR (a saved DUCK or COMP: its own); RESET KNOB the functions alone. EDIT held on the KICK menu: its classic view; the render's block buffers and the patterns in the pool (13.7 KB of RAM) | P1 | *Done; scenarios knob, maxout, kickmenu* |
| A-49o | **Kick roll** (LOAF BREAKER): the roll pads' K2 (was the break's filter) THRU: BREAK (as ever), KICK (the kick hit at the pad's rate from the next step, the pattern's kicks out meanwhile; the break left alone), BOTH; in the pad settings' spare byte (an older save: BREAK). The pad's light and view say KICK / ALL ROLL | P1 | *Done; scenario kroll.txt (a bar of R16 kick roll: 16 hits)* |
| A-49p | **A drum step's own** (LOAF BREAKER, SEQ): a step held on a drum lane, its keys acting as they come up (a knob turned meanwhile: no toggle), K1 NOTE (kick, hoover), K2 HITS (1–4 in its 16th), K3 CHANCE (100 % .. 12.5 %), K4 NUDGE (0 .. 7/16 late); the foot names them while held; the grid splits a tapping step, dims a chance's lower half, ticks a nudge. A byte a step (192 a pattern: the kick's 64, the others' 32) in a fifth store object, OBJ_STEP at 0xD8000 (an older save: none, the pattern's lane HITS of 0.110 on each step); copy, paste, undo, CLEAR take them. The render: a step's chance, its nudge and its repeats queued (16), the hits kept in order. K2 without a step held: DRUM FLT again | P1 | *Done; scenario hits.txt* |
| A-49q | **SEL + R8 / R16 / R32**: a kick roll for that press, whatever THRU says (THRU untouched) | P2 | *Done; scenario kroll.txt* |
| A-50a | **The output's last guard, a look-ahead limiter**: the mix delayed 64 samples (1.5 ms), its gain brought down smoothly before a peak over 0.95 arrives, back up over ~150 ms, what still passes rounded off softly under 1 (was: the gain cut at once on the peak sample, back over ~50 ms: steps heard as distortion with many sounds at once) | P2 | *Done; host renders compared (no change when it isn't working, no overs)* |
| A-50b | **RESET by five clicks, kick PRESET on OUT, the drums played on the big views** (LOAF BREAKER, 9 Nov 2026): ENV RESET picks with K1 or ALGORITHM and resets on five clicks of K2, K3 or K4, each within 2 s (five boxes; was K4 right twice); the KICK menu's OUT part K4 PRESET puts a built-in kick on the kick selected (was hidden on a layer FX's free knob); stopped, white keys 1–5 play the five drums anywhere in the KICK menu and on the drums' big views | P1 | *Done; scenarios reset.txt, kpreset.txt* |
| A-50c | **AUTO SAVE waits 3 s after the stop** (LOAF BREAKER, 9 Nov 2026): the save (a minute after a change) waits until the break has been stopped 3 s, so the flash's stall does not cut the stop's tail (a stutter heard when it saved at once). The drop moved from SEL ×3 to SEL + ENV the same day | P1 | *Done; scenario autosave.txt* |
| A-50d | **Break Swap** (LOAF BREAKER, 9 Nov 2026): the bank is 10 breaks, 357 KB: AMEN04 170 in CHOPPER's place, AMEN13 173 in DRUMMER's, AMEN15 174 and BREAKIN 175 after AMEN19 (the others keep their number on ALGORITHM). Every name in breaks.txt carries its BPM (11 characters at most) | P1 | *Done* |
| A-51d | **Break Sort** (LOAF BREAKER, 10 Nov 2026, chosen on a listening page of all 70 breaks as the FM-1 holds them): the bank is 9 breaks, 337.7 KB: THINK 114, NUSKOOL 140, UPDOWN 165, APACHE 115, THEWORM 110, AMEN19 172, AMEN10 135, AMEN20 164, IDM 169. AMEN04, AMEN13, AMEN15 and BREAKIN out (~20 KB of flash freed). Then (0.194) FLEXOR's order, and (0.195) AMEN15 174 back in IDM's place: AMEN10 135, NUSKOOL 140, AMEN20 164, AMEN19 172, UPDOWN 165, AMEN15 174, THINK 114, APACHE 115, THEWORM 110 (AMEN10 at power-on). Then (0.196) 10 breaks, 343.9 KB: NUSKOOL 140, AMEN20 164, AMEN19 172, UPDOWN 165, AMEN15 174, JUNGLE 175, THINK 114, APACHE 115, THEWORM 110, LO-FI 165 (one bar, as in CHOPPAH until 0.35); AMEN10 out. | P1 | *Done* |
| A-51e | **64 patterns** (LOAF BREAKER, 10 Nov 2026): P1–P64 in four groups of 16; one group in RAM (pats.p, kn, hits, steps.b; `set.pgrp`, `set.pattern` its slot), each group its own three store objects (OBJ_PG0/KG0/SG0 + g, 0xC0000–0xD7FFF). PRESETS, TRACKS items (K2: END, EMPTY, DROP, P1–P64), DROP AFTER and drops reach all 64; another group is read in (the sound plays on); leaving a group with changes not saved: saved first when stopped, refused while playing (SAVE OR STOP: a flash write cuts the sound). SAVE writes 6 objects; an older save's patterns are P1–P16 (written to their group at the first boot). | P1 | *Done* |
| A-51f | **HV KEYS** (LOAF BREAKER, 10 Nov 2026): SEQ, ALGORITHM past the last lane: the 27 keys (white and black) a chromatic piano for the HOOVER from C (K2 the octave, -2..+2); a key plays its note (perc_hit_pm); REC stopped writes it at the cursor (K1) and moves on, REC playing at the nearest 16th; K3 the cursor's note, K4 out (left) or a rest (right). Hoover notes -24..+39 (2 more bits a step, hvx, each group's OBJ_NG0 + g); a piano roll on the screen, the strip naming the black keys' notes. SAVE writes 7 objects. | P1 | *Done* |
| A-51g | **FILL's page** (LOAF BREAKER, 10 Nov 2026): SEL + ARP opens it (a GLO part without a tab, `GP_FILL`; SEL + ARP no longer writes a random pattern): K1 LATCH (ARP once on, again off), K2 DRUM (SNARE, CLAP, HAT, ALL 3), K3 LENGTH (1 BEAT, 2 BEATS, 1 BAR, 2 STEPS), K4 RATE (16THS, 8THS); `set.fill_cfg`, appended; 0 as before. | P1 | *Done* |
| A-51h | **MELODY** (LOAF BREAKER, 10 Nov 2026, FLEXOR's picks on a listening page): the HOOVER's voice (PV_STAB) is now the pattern's melody instrument, ACID, LEAD, PAD or REESE (perc.c: notes held to the next one, ACID's slides and accents, legato glides), one a pattern (hv.inst, the group's OBJ_NG0 + g), each instrument its own sound (smem[0..3]; pstab the one playing). SEQ K2 on a drum lane: what it plays (KICK 1–10, the drum's memory, the MELODY's instrument; the drums' filter: HOME K3 and SEL + SELECT only). MIXER: LEVELS K4 MELO (set.mel_vol), FX on GROOVE K3. KEYS (was HV KEYS) names the instrument. LO-FI 165 out of the bank. | P1 | *Done* |
| A-51i | **Pages by domain** (LOAF BREAKER, 10 Nov 2026, 0.201; 0.200 the layout before): each button a domain named on the screen, LIVE (HOME), SOUNDS (EDIT: sources KICK, DRUMS, MELODY, BREAK; SEL + EDIT the next, EDIT the source's next part), PATTERN (SEQ: lanes, KEYS, GROOVE = SWING + FILL's LATCH, DRUM, LENGTH), SONG (ENV), MIX (LFO: LEVELS, SENDS = FX + the drums' route, MASTER, DYNAMICS), EFFECTS (FX), SYSTEM (GLO: MIDI, SETUP, EFFECTS, FX SET, RESET with EVERYTHING = RESET ALL, 15 turns). The break's PITCH, GRAIN, SNAP (and LEVEL) are SOUNDS › BREAK (GLO's PITCH code, `GP_BREAK`), GROOVE is GLO's `GP_FILL` shown under SEQ. SEL tapped: one table of SEL + a button. | P1 | *Done* |
| A-50e | **Two-colour skins** (LOAF BREAKER, 9 Nov 2026): SYNTHWAVE (neon cyan values, hot magenta, indigo grounds), VAPOR, TOXIC, MIAMI, LAVA, GOLD, BLAZE (flashy orange and red): 17 skins, GLO › MORE K1; the swatches in two rows of nine | P2 | *Done; scenario skins.txt* |
| A-50f | **Screen EFFECTS, the skin on K4, an emptied case** (LOAF BREAKER, 9 Nov 2026): GLO MORE: K1 the effect's AMOUNT (settings `scrfx`, appended), K2 AUTO SAVE, K3 EFFECTS (CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT; `crt` its index, an older save's ON = CRT), K4 the skin. Done on every UI blit (common.c crt_blit); the kick's and the drop's moments redraw the whole screen as they start and end. TRACKS K2: EMPTY left of P1 (a gap kept; ENV held deletes it) | P2 | *Done; scenarios effects.txt, trkempty.txt* |
| A-50g | **LFO + K1–K4** (LOAF BREAKER, 9 Nov 2026): LFO held, on any page: K1 the break's level, K2 the kick's, K3 the drums', K4 the FX's (the MIXER's LEVELS), the page kept; LFO acts as it comes up, nothing after a turn | P1 | *Done; scenario lfomix.txt* |
| A-50h | **WAVE ZOOM** (LOAF BREAKER, 9 Nov 2026): GLO MORE, PRESETS: how much of the past HOME's scrolling wave (cdj.c) holds, 1/2, 2/3, 5/6, 1, 1 1/4, 1 1/2, 2 bars (settings `wzoom`, appended, 3 one bar). The scope ring holds 2 bars (480 columns, 1.9 KB), in RAM now, the pool being full | P2 | *Done; scenario wavezoom.txt* |
| A-50i | **LFO + K: bigger steps; GLOBAL without the waveform** (LOAF BREAKER, 9 Nov 2026): LFO + K1–K4 move the levels 5 % a detent (the MIXER's page: 2 %). GLOBAL's pages leave the top waveform out: MORE lays out SKIN and its swatches there, then EFFECTS (what each does), WAVE ZOOM, AUTO SAVE | P2 | *Done; scenarios lfomix.txt, effects.txt* |
| A-50j | **GLO EFFECTS, the splash's aberration** (LOAF BREAKER, 9 Nov 2026): GLO's fourth part: K1 AMOUNT, K2 ON A KICK, K3 AT THE DROP, K4 LENGTH of the screen effect (settings `sfx_kick`, `sfx_drop`, `sfx_len`, appended, 50 each: as before). The splash and its glitch go through RGB, the red 2 pixels off, up to 8 as it jumps | P2 | *Done; scenario sfxset.txt* |
| A-50k | **Screen effects, several at once** (LOAF BREAKER, 9 Nov 2026): GLO EFFECTS (its fourth part): white keys 1–7 turn CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT on and off, any of them together (settings `sfx_on`, a bit each, appended; none to start: the site's screenshots clean); K1 points, K2 on/off. FX SET (fifth) their settings. MORE keeps AUTO SAVE, SKIN, WAVE ZOOM. PRESS PLAY's picture: a bar of APACHE and SUB DRIVE kicks played in the host simulator (app/ui/pp_wave.h, tools/make_pp_wave.py) | P2 | *Done; scenarios effects.txt, sfxset.txt* |
| A-50l | **SEL lock** (LOAF BREAKER, 9 Nov 2026): SEL x2 (two quick clicks) locks it down: lit, it counts as held for every combination; pressed again it is free (that press no click). The effect's reset (was SEL x2) is SEL + REC on FX and EDIT KICK's FX | P1 | *Done; scenario sellock.txt* |
| A-50m | **SEL + SELECT: the drums' filter** (LOAF BREAKER, 9 Nov 2026): SEL + SELECT turns DRUM FLT (HOME's K3) on any page (was the kick's memories KICK 1-10: PRESETS on the KICK menu). PRESETS during a track picks the pattern as ever (a DRUM FLT there, tried in 0.167, undone) | P1 | *Done; scenario trkflt.txt* |
| A-50n | **Black keys: KIL, GATE off** (LOAF BREAKER, 9 Nov 2026): R8 R16 R32 REV FLT HLF VNL RND ARP KIL SUB: GATE has no key (HOME's K1 gates), RANDOM on the 8th, the ARP key on the 9th, KIL on the 10th: held, the sequencer's kicks quiet (`kick_hold`; a kick roll still plays) | P1 | *Done; scenarios kill.txt, pads.txt, arp.txt* |
| A-50o | **Kick tools** (LOAF BREAKER, 9 Nov 2026): SEL + knob functions TUNE, DECAY, REV BASS, GLIDE, CLEAN SUB, MORPH (SK 9-14); CLEAN SUB (`csub`, kick.c: the SUB's sine after all the distortion, the distorted lows out); KICK MORPH (`kmorph`, `kmorph_to`: app.c kick_send, the kick sent between it and another memory); TERROR + TYPE RECT and TUBE (KICK_T_TYPE 3, 4) | P1 | *Done; scenario kicktools.txt* |
| A-50p | **KNOB per kick** (LOAF BREAKER, 9 Nov 2026): each kick memory (KICK 1–10) keeps its SEL + knob functions and their settings: ENV KNOB's choices, CLEAN SUB, BASS (GLIDE, REV BASS, START, CURVE), MORPH and its target (steps.kx, the steps' object, appended; an older save: every kick with the knobs it had). Swapped with the kick (kick_mem_turn); RESET KICK clears its CLEAN SUB and MORPH, ALL KICKS every kick's knobs; KNOB shows whose | P1 | *Done; scenario kickknob.txt* |
| A-50q | **EDIT KICK › KNOBS** (LOAF BREAKER, 9 Nov 2026): a fifth section on the KICK menu, under the kick's picture: KNOBS (K1–K4 what SEL + K1–K4 turn, each function's first setting), K1 SET .. K4 SET (that knob's function, all its settings, as ENV KNOB; its name by the kick's; PRESETS there gives the knob another function, selk_assign_k). The kick playing's (A-50p) | P1 | *Done; scenario kickknobs.txt* |
| A-50r | **CLEAN MORPH** (LOAF BREAKER, 9 Nov 2026): SYNTH's part after BASS on the KICK menu: K1 CLEAN SUB, K2 MORPH, K3 TO KICK, the kick's own with no knob given them (every KNOB function now has its place on the KICK menu) | P1 | *Done; scenario cleanmorph.txt* |
| A-50s | **AUTO SAVE waits for silence** (LOAF BREAKER, 9 Nov 2026): stopped, it also waits until the output (after MASTER) has been under −60 dB for 3 s (out_sound, set by the render): a long tail, a reverb, an echo, a drum played by hand is never cut by the flash's stall | P2 | *Done; scenario autosave.txt* |
| A-50t | **Easier to learn, nothing lost** (LOAF BREAKER, 9 Nov 2026, tested as TEST 9.04): ENV the song alone (TRACKS, PLAY, DROP, BUILD; KNOB out: EDIT KICK › KNOBS has the functions); RESET on GLO (sixth part, RST_ON); GLO MORE K1 MODE (set.novice: BEGINNER passes over the KICK menu's advanced parts, KP_ADV; a new FM-1, the web emulator and RESET ALL start BEGINNER, an older save EXPERT, the simulator EXPERT); the SEL card: SEL's combinations everywhere above, the page's own below; the KICK menu's parts say what they are for; the manual opens with "Start in five minutes" (the hub card's cheat link); an eighteenth skin, ACID | P1 | *Done; scenarios novice.txt and the moved ones* |
| A-50u | **A lighter interface, measured** (LOAF BREAKER, 9 Nov 2026, tested as TEST 9.05): GLO MORE K3 METER (CPU: the render's share, average and peak, plat_cpu_pct; IMG: strips sent; FR: passes a second); the screen's rows remembered as hashes (rows_send): a full-width strip sends only its changed rows, none if none (the header's BPM and kick line alone on a beat); the CDJ view and the MIXER at 20 fps; the kick's picture worked out at most every 100 ms; a pass's changes applied once, and a kick or FX knob applies its own part alone (changed_kick, changed_fx); a 30-minute random soak in the simulator at every release (tools/soak.py) | P1 | *Done; every scenario's screens pixel-identical with and without the row cache; soak.py seeds 1-3, 7, 11, 42, 99* |
| A-50v | **SEQ's hands** (LOAF BREAKER, 9 Nov 2026): K3 on a drum lane only CLEAR (left) or 4X4 (right); playing, the lane shown follows the bar playing on a lane of 32 or 64 steps (ui.seq_manual: ALGORITHM or SEQ again stops it until a stop); SEQ held + white key 1–4: the bar shown copied there (steps, HITS / COND / NUDGE, notes); SEQ held + K1: the lane shifted a step, round; SEQ held + REC: every drum lane cleared, the break kept; REC on a drum lane while playing: a white key writes its drum at the nearest 16th (midi_drum); the SEL card's SEQ part names them | P1 | *Done; scenario seqtools.txt* |
| A-50w | **KIL's LATCH** (LOAF BREAKER, 9 Nov 2026): KIL held, K1 its LATCH (set.half_mt_off bit 2, saved; RESET PADS off): ON, a press quiets the sequencer's kicks, the next brings them back; turned on while held, they stay quiet | P2 | *Done; scenario kill.txt* |
| A-50x | **Kicks copied, each track item its kick** (LOAF BREAKER, 10 Nov 2026): OUT's K4 PRESET past the built-in kicks: COPY K1..K10, that memory's kick, SYNTH and knobs (kx) onto the kick selected, its volume kept; TRACKS: ALGORITHM the item, K1 its kick (SAME, KICK 1–10; steps.tk, appended to the steps' object), switched with kick_mem_go as its pattern comes in (at once when played from it, with the drop into it), its cell showing "K7" | P1 | *Done; scenario kickcopy.txt; track, trkgap, trkfrom, trkempty on ALGORITHM* |
| A-50y | **A drop's own kick** (LOAF BREAKER, 10 Nov 2026): ENV DROP's PRESETS BUILD KICK (set.drop_kick, appended; RESET DROP: SAME) and a track's DROP item's K1: the kick its build plays (its kick roll), taken as the build begins, the one before back at the drop (or at a stop), the pattern dropped into's own after it | P2 | *Done; scenario dropkick.txt* |
| A-50z | **Early next gen on the knobs, and styles** (LOAF BREAKER, 10 Nov 2026): six more SEL + knob functions: PITCH DROP (SYNTH DROP, TIME), RUMBLE (its amount, time), PUNCH (MACRO's), DRUM FLT (set.kick_dj), HOLD (SYNTH's), KICK FX (the kick's two slots' mix); codes past 15 with a fifth bit (set.selk_hi, appended; steps.kxh per kick memory, appended); EDIT KICK › KNOBS' STYLE part: K1–K4 each their knob's function, PRESETS one of eight styles (TERROR, EARLY NEXT GEN, HARDCORE, NEW SCHOOL, RAWSTYLE, INDUSTRIAL, FRENCHCORE, LIVE BUILD), the four functions at once, CUSTOM once changed | P1 | *Done; scenario knobstyle.txt* |
| A-51o | **TUTORIAL** (LOAF BREAKER, 10 Nov 2026, 0.211; 0.212: 19 steps, HOME first; 0.223: 18, the built-in kicks, 5 s between steps): 19 steps in seven lessons (HOME: PERFORM, the break, effects, the kick and its built-in kicks (OUT's K4), DISTO, the pattern, the drop, SAVE) on the FM-1 itself (ui/tutor.c): the header says what to do, the step's buttons or keys blink, done: "OK!" and the next, at once for a button, or "OK!  NEXT IN n S" and 5 s later (TUT_WAIT, 0.213: only the steps with something to hear, TUT[].wait) (counted: HOME, the built-in kicks, white and black keys, the kick's knobs, SEL + K4, SEQ's steps, SAVE). SETUP K3 MODE (0.213): TUTORIAL left of BEGINNER; a new FM-1 / RESET ALL start it (set.tut, appended; an older save: off); GLO quits, SEL + GLO skips. Also: app_frame read st.playing before brk_status filled it (the build's kick sometimes back at once): playing_now() | P2 | *Done; scenario tutorial.txt* |
| A-51p | **SETUP in the knobs' order** (LOAF BREAKER, 10 Nov 2026, 0.213, FLEXOR): SYSTEM › SETUP's rows top to bottom are K1 SKIN, K2 WAVE ZOOM (PRESETS too), K3 MODE, K4 AUTO SAVE; the METER (A-50u's) moves to SYSTEM › MIDI K3, EXPERT only (BEGINNER: none, and turned off) | P2 | *Done; scenarios skins, autosave, novice, tutorial* |
| A-51q | **EDIT ZOOM** (LOAF BREAKER, 10 Nov 2026, 0.216, FLEXOR): SYSTEM › EFFECTS, PRESETS: the time EDIT's pictures show, the kick's and the drums' (set.ezoom, appended): AUTO (as before: the kick 500 / 250 ms, KICK VIEW PART each part's own; a drum its decay × 1.4) or 250 MS, 500 MS, 1 S, 1.5 S, 2 S for all; the time shown under the kick's HZ; RESET SCREEN: AUTO. The picture's work grows with its time (main loop only, never the sound) | P2 | *Done; scenario ezoom.txt* |
| A-51s | **BEGINNER, the essentials** (LOAF BREAKER, 11 Nov 2026, 0.223, FLEXOR: "surf the machine first, the depth after"): the KICK menu is one page, OUT's (kick_easy: K1 PRESET, K2 DISTO, K3 TUNE, K4 TAIL; EDIT again and ALGORITHM stay); the FX page's PRESETS picks a whole SCENE (fx.c SCENE[]: CLEAN, PUNCH, DUB, LO-FI, CRUSHED, INDUSTRIAL, SPACE) and FX again stays on MAIN; the TUTORIAL 18 steps (the kick's K1 PRESET; no ALGORITHM to OUT). 0.224: SEQ's drum lanes K1 BEAT (seq.c BEAT[]: the four lanes at once), no step menu, no LENGTH; SONG each part's essentials (track.c trk_knob_ok); the held keys' knobs off, the pads' too but ARP's K1 (OVL_EASY, six) and K2; BREAK its LEVEL, GROOVE its SWING (global.c glo_knob_ok); a knob left out drawn blank. Next: the drums PRESET + FX (docs/sounds/PERC_PRESETS.md) | P2 | *Done but the drums; scenarios fxscene, beginner, novice, tutorial* |
| A-51t | **The drums' extras and FLEXOR's presets** (LOAF BREAKER, 11 Nov 2026, 0.225): perc.c perc_set_x: PX_DROP (the snare's body x1.7..x6), PX_DTIME (3..140 ms), PX_DIRT (SOFT, HARD, FOLD, BITS), PX_LIFT (an upward compressor, its gain every 16th sample), PX_ROOM (a 4-comb, 2-allpass room in 16-bit lines, 5.8 KB, gated 40..540 ms; one for the drums), all 0 as before; kept per memory in smem (the MELODY's, 240 bytes; set.pxv appended marks it). UI: SHAPE K4 DIRT, a third page MORE; BEGINNER's one page PRESET, FX (DRY, ROOM, CRUSH, SMASH, SMASH+BITS), LEVEL, TUNE; the snare's presets 4 + 9 (SHRAPNEL .. GABBER BITS), the clap's 4 + 2 (perc.c PP_SN, PP_CP). The picture still draws SOFT | P2 | *Done; scenario drumfx* |
| A-51u | **EDIT goes round every sound (taken back in 0.227: A-51v); lighter render** (LOAF BREAKER, 11 Nov 2026, 0.226, FLEXOR: "SEL + EDIT to go from the kick to the snare is too complicated"): EDIT again past the KICK's last section (BEGINNER: its one page) opens the SNARE, then HAT, CLAP and the KICK's first section; SEL + EDIT stays the sources' shortcut. Render: the drums' quiet voices skipped, the kick's and the drums' maths inlined (BRK_ENGINE in kick.c, perc.c), the DUB delay at rest once silent, the CDJ view's scope only on HOME / GEN / KICK, the scenes CRUSHED and INDUSTRIAL without MB OTT; OVERLOAD says CPU FULL when no slot was there to bypass | P1 | *Done (the scenario edit_round gone with the round)* |
| A-51v | **BEGINNER: the kick's LENGTH, MAGIC EQ / MBC; EDIT stays on the KICK** (LOAF BREAKER, 11 Nov 2026, 0.227, FLEXOR): SEQ K4 LENGTH on a kick lane 16, 32, 64; on a drum lane nothing ("16 STEPS / LONGER: EXPERT MODE"); BEGINNER turned on (SETUP K3) sets the group in RAM's patterns to a 32-step kick and 16-step drums (app.c novice_lengths; their second bars kept), and a new pattern in BEGINNER starts at 32 (pat_defaults, grp_blank). The FX page's PRESETS past SPACE: MAGIC EQ, MAGIC MBC on the slot shown alone (fx.c scene_slot). A-51u's EDIT round taken back: EDIT on the KICK goes round its sections only (BEGINNER: its page), on a drum SNARE HAT CLAP; SEL + EDIT the other sources | P2 | *Done; scenarios beginner, fxscene* |
| A-51w | **BEGINNER's KICK on two pages** (LOAF BREAKER, 11 Nov 2026, 0.228, FLEXOR: "the most relevant, SUB DRIVE on the first, TUNE on the second"): on OUT's part, KICK (K1 PRESET, K2 DISTO, K3 SUB DRIVE: the SUB layer's DRIVE effect, its GAIN, OFF at the left; K4 TAIL) and SHAPE (K1 TUNE, K2 PUNCH: MACRO's, K3 CLICK: its level, K4 P DROP: SYNTH's DROP); EDIT again and ALGORITHM the other page (kick.c KEASY[], keasy_pg; from another page, KICK); the tabs and the foot name them | P2 | *Done; scenario novice* |
| A-51x | **BEGINNER as FLEXOR chose it** (LOAF BREAKER, 11 Nov 2026, 0.229, a checklist page and four questions): the KICK three pages (KICK: PRESET, DISTO, SUB DRIVE, DRIVE; SHAPE: P DROP, DROP TIME, TAIL, TAIL MODE; MIX: the layers' levels SUB, PUNCH, GRIT, CLICK); the drums' page PRESET, FX, LEVEL, DECAY; SEQ's K1 FILL again, the BEAT presets gone; the FX chain two slots (the third and fourth muted and hidden: fx_apply sends them off), PRESETS a short list (FX_EASY) then MAGIC EQ, MAGIC MBC, the scenes gone; MB KICK and MB OTT removed from the firmware (a slot saved as one: a plain MB COMP); the ARP key all sixteen effects; DST and SUB keys say EXPERT MODE (terror_off ignored); a blank knob says EXPERT MODE; a new FM-1 (and RESET ALL) asks TUTORIAL / BEGINNER / EXPERT after the start screen (set.tut TUT_ASK 0xFD) | P2 | *Done; scenarios novice, beginner, fxeasy, firstboot (LOAF_ASK=1); mbkick, fxscene gone* |
| A-51y | **BEGINNER's sounds in the header** (LOAF BREAKER, 11 Nov 2026, 0.230, FLEXOR: the KICK's pages were shown twice): on the KICK's pages and the drums', the header's tabs are KICK, SN, HH, CP, "SELECT: KICK SN HH CP" at the right; SELECT goes round them in either view; the KICK's pages stay the tabs at the picture's foot | P2 | *Done; scenario novice* |
| A-51z | **One view for the kick and the drums; BEGINNER's SEQ without GROOVE** (LOAF BREAKER, 11 Nov 2026, 0.231, FLEXOR): the drums' big view has the KICK's RGB and WAVE views (perc.c perc_rgb, into kick.c's kv_rgb: never on the screen together), EDIT held going RGB, WAVE, CLASSIC on either page, the state shared (ui.kview, kclassic, pbig); BEGINNER's SEQ: ALGORITHM / SEQ round the lanes, GROOVE not between (SEL + ARP still) | P2 | *Done; scenarios drumview, kickmenu, beginner* |
| A-52a | **BEGINNER's black keys with their knobs** (LOAF BREAKER, 11 Nov 2026, 0.232, FLEXOR): a black key held turns its knobs as in EXPERT (pad_knob_name, pad_knob_turn: no BEGINNER limit, the ARP key's MORE too); DST and SUB do nothing and are never lit in BEGINNER (their LEDs and the screen's keys), their functions the KICK page's knobs (the DST mute ignored, SUB DRIVE as its knob sets it) | P2 | *Done; scenario fxeasy* |
| A-52b | **BEGINNER's SEQ shows the break** (LOAF BREAKER, 11 Nov 2026, 0.233, FLEXOR: "no need for that room in BEGINNER, and the function isn't hidden"): the break's two halves (BRK1, BRK2) always the grid's top rows (seq_rows), the drums under them; EXPERT as before (the half the keys edit, only while they edit it) | P2 | *Done; scenario beginner (bg_seq.png)* |
| A-52c | **EXPERT's SEQ: BREAK and its rows shown** (LOAF BREAKER, 11 Nov 2026, 0.234, FLEXOR): ALGORITHM up past the first lane reaches BREAK (ui.seq_brk; the tab and the header say BREAK; the white keys play slices), its K4 STEPS the break's rows the grid shows: HIDDEN, 16 (BRK1), 32 (BRK1, BRK2; to start), kept in settings' brkv (the MELODY's mel_ver byte, tagged 0xB0: the settings at their 3840 bytes; an older save: 32); hidden rows skipped by ALGORITHM and by the bar-following; the pattern plays 32 steps whatever; above BREAK, GROOVE as before | P2 | *Done; scenarios brkview, domains* |
| A-52d | **The TUTORIAL for BEGINNER as it is** (LOAF BREAKER, 11 Nov 2026, 0.235, FLEXOR: "update the tutorial"): 23 steps in eight lessons (tutor.c TUT[]): THE BREAK (HOME, PLAY, slices, ALGORITHM another break), EFFECTS (a black key, ARP held + K1: its effect; the ARP key blinks), THE KICK (EDIT, K1 the built-in kicks, K2 DISTO, EDIT: SHAPE, K3 a longer tail, PRESETS KICK 2), THE DRUMS (SELECT: the SNARE, K1 its preset, K2 its FX), THE PATTERN (SEQ, two kicks, a snare), THE FX (FX, PRESETS an effect), THE DROP, SAVE. Gone: stop / play again, a second black key, K2-K4 "shape it", SEL + K4 DISTO | P2 | *Done; scenario tutorial* |
| A-52e | **RESET ALL as a first start** (LOAF BREAKER, 11 Nov 2026, 0.236, FLEXOR): after its own screen (LOBOTOMIZED), the screen's state as at power-on (ui cleared, app_ui_init): the start screen, then the mode asked (TUTORIAL / BEGINNER / EXPERT), as on a new FM-1 | P2 | *Done; scenario resetask (LOAF_ASK=1)* |
| A-52f | **The 1.0's skins** (LOAF BREAKER, 11 Nov 2026, 0.237, FLEXOR): 24, picked from 48 rendered on the simulator (the 18 before and 30 two-colour proposals), sorted by close colours, named and ordered by him: CHROME, FIRE&ICE, CHOPPAH (AMBER, the start), GLOUPI (LAVA), TESS, OUTRUN, THUNDER, CYBERPUNK, MC SALO, ACID, GARRY, TOXIC, PIP-BOY, NEON, BLACKLIGHT, PATINA, SYNTHWAVE, MIAMI, ELECTRIC, ARCTIC, BREADCRUMBZ (WHITE), IRIS, GLUCKSMAN (0.238; ULTRAVIOLET), MAXENCE; names at most 8-9 letters (the knob's column; 0.238: a value wider than it drawn smaller); SETUP's swatches two rows of twelve; set.skin saved with bit 7 (SKIN_NEW), an older save's place mapped to the nearest kept (SKIN_OLD); RESET SCREEN: CHOPPAH | P2 | *Done; scenario skins* |
| A-52g | **RESET by K4, K2 and GLO; the skins' full names** (LOAF BREAKER, 11 Nov 2026, 0.239, FLEXOR): on SYSTEM › RESET, ALGORITHM goes to SYSTEM's parts as on its other pages; K4 picks what; K2 alone counts the five clicks (EVERYTHING too, its fifteen gone), then GLO within 5 s resets (ui.rst_ask, rst_confirm; not pressed or the page left: CANCELLED). The skins' names whole again (BLACKLIGHT, CYBERPUNK, SYNTHWAVE, BREADCRUMBZ): a foot value wider than 54 px drawn in FONT_T, 2 px to the left | P2 | *Done; scenarios reset, kreset, resetask, site* |
| A-52h | **RED back as the default skin** (LOAF BREAKER, 11 Nov 2026, 0.240, FLEXOR): TOXIC renamed GARRY; the first GARRY (JUNGLE's colours) gone, RED (the red from the start) in its place, the 11th, and LOAF BREAKER's default again (new settings, RESET SCREEN, an older save's RED) | P2 | *Done; scenario skins* |
| A-52i | **Slimmer break rows on SEQ** (LOAF BREAKER, 11 Nov 2026, 0.241-0.242, FLEXOR): BRK1 and BRK2 as tall as the grid's average row (its height over its rows), between 18 and 24 px (0.241's 13 too small: the number through its box), the number centred above the reversed / ratchet mark; the drums' rows take the rest | P2 | *Done; scenarios seq, beginner, brkview* |
| A-52j | **The OVERLAY page back on ARP; FILL gone** (LOAF BREAKER, 11 Nov 2026, 0.244, FLEXOR: "the screen from before with all the FX, and remove the FILL"): ARP opens the OVERLAY page again (ovl.c: sixteen effects, one a white key, held or LATCH; K1 AMOUNT, K2 RATE, K3 SWEEP, K4 LATCH); SEL + ARP: GROOVE, SWING alone; FILL removed (the engine's roll, ARP held, its settings; set.fill_cfg unused), COND FILL / NOT FILL no longer offered (a step saved so plays always); the TUTORIAL's EFFECTS lesson two steps more (ARP: the page; two of its effects): 25 | P2 | *Done; scenarios overlay (CI), tutorial, loaftest; fillmenu gone* |
| A-52k | **BLACK KEYS: layouts** (LOAF BREAKER, 11 Nov 2026, 0.244 with A-52j, FLEXOR: "Choppah sera l'unique mode du mode begginer ... Flexor Custom est le layout actuel, il est très important de le garder comme tel"): each black key a function (app.c BKF_*: the eleven pads, FREEZE and DUB back from CHOPPAH, then ARP, KIL, DST, SUB); presets CHOPPAH (R8 R16 R32 REV FRZ HLF VNL GAT RND FLT DUB: a new FM-1's, RESET ALL's, BEGINNER's only; turning BEGINNER on sets it, EXPERT keeps it), FLEXOR LAYOUT (0.244's, unchanged), CUSTOM; ARP's tabs in EXPERT (ARP again or ALGORITHM): ARP EFFECTS, BLACK KEYS (K1 PRESET, K2 KEY or a black key pressed, K3 EFFECT; on a preset K3 copies it to CUSTOM), MORE (to come); stored in the STAB's old place (settings bkeys, BK_TAG); BEGINNER's DST / SUB restrictions gone; the TUTORIAL's ARP-key step removed (0.246, FLEXOR: not relevant; 24 steps); 0.246 (FLEXOR): the TUTORIAL's ARP-key step gone (24 steps), BEGINNER's kicks 16 steps (were 32: no step puts a kick in before the kick lesson; K4 16, 32, 64 as before) | P2 | *Done; scenario blackkeys (CI), beginner, tutorial* |
| A-52l | **TUTORIAL: the way back** (LOAF BREAKER, 11 Nov 2026, 0.248, FLEXOR: "si je change de menu pendant le tutoriel, au lieu de l'étape en cours afficher la commande pour aller à l'endroit"): off a step's page (or tab, lane), the header's second line says how to get back (tutor.c tut_away: PRESS HOME / ARP / EDIT / SEQ / FX, TURN SELECT, TURN ALGORITHM) and that button alone blinks; back there, the step again; 0.249: a page button the step doesn't ask for is held back ("FOLLOW THE STEP"; tut_btn_ok: its own, the way back's, the next step's once done; GLO, PLAY, SAVE, REC never), the steps say KNOB 1 .. KNOB 3; 0.250: AUTO SAVE off while the TUTORIAL runs, on at its end or quit, on when BEGINNER is chosen at the first start (EXPERT: as it was); 0.252 (FLEXOR, on a checklist): 26 steps, THE PATTERN (two kicks, a snare) before THE KICK, which stops the break (PLAY), plays the four drums on keys 1-4 and has each change heard on key 1 (the snare's on key 2: tut_heard, a key after the last knob turn); THE FX starts PLAY again; EDIT from SEQ during its step opens the kick | P2 | *Done; scenario tutaway (CI)* |
| A-52m | **The KICK menu opens on ATTACK's PRESET** (LOAF BREAKER, 11 Nov 2026, 0.253, FLEXOR: "avant l'onglet click dans attack avoir un onglet preset avec juste le réglage des preset"): ATTACK's parts PRESET, CLICK, PUNCH, STACK; PRESET (kick.c KP_PRESET, KP_PART_PRESET) K1 the built-in kicks alone; EXPERT's EDIT opens on it (ui.ksect 0; BEGINNER's KICK page unchanged) | P2 | *Done; scenario kickmenu (CI), the others an EDIT more to BODY* |
| A-51r | **MAGIC EQ and MAGIC MBC** (FLEXOR's presets, 10 Nov 2026, set by ear on the FM-1): the PRESETS list's last two, past CLIP (FX chain and KICK FX). MAGIC EQ: LOW −5.8 dB at 182 Hz, MID −2.9 dB at 999 Hz Q 1.59, HIGH +1.9 dB at 8 kHz, 0 dB (marked in its extras' rsv[0]: 1). MAGIC MBC: 200 Hz / 2.5 kHz, hard knee, 0 dB out; LOW 0 dB 8:1 10/150 ms −6 dB, MID −34 dB 3:1 5/100 ms, HIGH −48 dB 2:1 2/60 ms +2 dB; amounts 0.5, 0.35, 0.5 (4.5:1, 1.7:1, 1.5:1); 60 % wet (its detail's rsv[0]: 3). No new setting | P2 | *Done; scenario magic.txt* |
| A-51n | **Kicks and drums only** (LOAF BREAKER, 10 Nov 2026, 0.209, FLEXOR's choice): the MELODY (0.200-0.208: ACID, LEAD, PAD, REESE) and KEYS (SEQ's piano, 0.198) removed: perc.c's fourth voice (PV_STAB) empty, its place kept for perc_t's layout in the saves; DR_STAB out of the drums (SEQ's V1/V2 lanes, the big view, SEL + EDIT's sources, MIDI note 40, key 5), MIX LEVELS K4 empty, the melody's notes (OBJ_NG0 + g) no longer saved (SAVE writes 7); pats.hits keeps its five (NDRUMS_SAVED). RESET: after a reset, its page's knobs do nothing for 3 s (DONE shown) | P1 | *Done; scenarios hvkeys, melody removed; reset.txt checks the 3 s* |
| A-51m | **SERATO's colours, KICK VIEW** (LOAF BREAKER, 10 Nov 2026, 0.206): GLO EFFECTS K3 WAVE (set.wstyle, appended): the KICK menu's RGB picture and HOME's CDJ view in REKORDBOX's colours (the bands mixed) or SERATO's (serato_col: one hue, red to blue, the lows weighed three times); K4 KICK VIEW (set.kfocus, appended): the picture the WHOLE kick or the PART edited (a layer alone; CLICK 50 ms, PUNCH and STACK 150 ms, STACK its attack's kick; the TAIL section 1 s) | P2 | *Done; scenario kfocus.txt* |
| A-51l | **The KICK menu by anatomy, the kick in RGB** (LOAF BREAKER, 10 Nov 2026, 0.205): sections ATTACK (CLICK, PUNCH, STACK), BODY (MIX, SUB, GRIT, SYNTH, SYNTH +, MACRO, OUT; opened first), TAIL (TAIL, TAIL +, BASS, MORPH), DIRT, TONE, KNOBS (+ TOOLS); the section's parts in a row at the picture's foot (the layers' levels there before: MIX has them); the picture's views, EDIT held: RGB (kick_rgb: the CDJ view's bands, followed ~15 ms, 500 ms, squared for colour), WAVE, CLASSIC | P2 | *Done; scenario kview.txt* |
| A-51k | **KICK LAB** (LOAF BREAKER, 10 Nov 2026): the KICK menu's sixth section, LAB (kick.h kick_lab_t, each kick's own, OBJ_KLAB 0xB6000): TAIL (MODE CLASSIC/CLEAN: the distortion before the SUB's fall, PUNCH and GRIT on its octave, GRIT's FM harmonic, the RUMBLE's loop on the note; NOTE A#0..A2; CURVE EXP/LINEAR/SNAP; END 40..1000 ms), TAIL + (a closing two-pole low-pass, its time, TAIL DRIVE), STACK (the attack from KICK n, a second kick voice until the hand-over: CROSS, FADE, SOLO), TOOLS (MUTATE with undo, A/B against the kick as it came or was saved); all 0: bit for bit as before | P1 | *Done; scenario klab.txt, engine test_kick_lab* |
| A-51j | **PRESET 1–4 on STYLE** (LOAF BREAKER, 10 Nov 2026): after the eight styles, four of FLEXOR's own: the four knobs' functions and values (steps.kpre, appended: codes, 0..200, stored), the same for every kick; PRESETS onto a stored one: the kick playing takes them (values turned there quietly, sk_set_norm); onto an empty one: the knobs as they were before PRESETS turned; SAVE on STYLE with one picked stores the knobs in it, then saves all | P2 | *Done; scenario knobpre.txt* |
| A-51a | **TUNE off the knobs** (LOAF BREAKER, 10 Nov 2026): not a SEL + knob function any more (sk_live; a knob given it: its own again; SYNTH +'s K4 keeps it); styles without it: HARDCORE DRIVE, PUNCH, DECAY, TERROR; RAWSTYLE REV BASS, GLIDE, PITCH DROP, T+ TAIL; FRENCHCORE DRIVE, DECAY, TERROR 2, PITCH DROP; NEW SCHOOL DRIVE, CLEAN SUB, T+ TYPE, TERROR (a style's DRIVE on K1, its TERROR on K4) | P2 | *Done; scenarios kicktools, kickknob, knobstyle* |
| A-51b | **Black keys reordered** (LOAF BREAKER, 10 Nov 2026): R8 R16 R32 REV RND HLF VNL ARP KIL TER SUB: FILTER no key (HOME's K2 has the DJ filter), RANDOM on the fifth, the ARP key on the eighth, KIL on the ninth (KILL_KEY), the tenth TER (TERR_KEY), as SUB's DRIVE: a press, the kick's TERROR on (an amount at nothing: 50 %) or off whatever its amount (selk_hi bit 4: the kick memory's own, saved; RESET KICK: on); lit while the TERROR sounds | P2 | *Done; scenarios pads, arp, kill* |
| A-51c | **Friendlier names, a lighter BEGINNER** (LOAF BREAKER, 10 Nov 2026): on the screen and in the docs TERROR is DISTO, TERROR 2 SCREAM, TERROR + DISTO + (its settings D+ TYPE, D+ SCREECH, D+ TAIL, D+ DRIVE), the black key TER DST (the code keeps TERROR's names; the style TERROR, a genre, keeps its own); DST as SUB's DRIVE: the kick's own, saved (selk_hi bit 4), on with 50 % from nothing; BEGINNER also passes over the KICK menu's MACRO and BASS and the MIXER's MASTER and DYNAMICS (LFO: LEVELS, GROOVE) | P2 | *Done; scenarios novice, pads* |
| A-49z | **The ARP black key's knobs** (the OVERLAY page gone): MAIN, EFFECT, AMOUNT (that effect's), TIME (RATE or SWEEP, as the effect has), LATCH; FX while it is held: MORE, THRU, ON / OFF, RATE, SWEEP. **The GLOBAL page without Bluetooth**: MIDI over USB and the TRS jack, K3 and K4 nothing. **The control map and the manual's tables** as the firmware is (the black keys R8 R16 R32 REV FLT HLF VNL GAT RND ARP SUB, every page's knobs, SELECT's roles) | P2 | *Done; scenario arp.txt* |
| A-49y | **The drums' big view**: on the KICK menu's big view SELECT goes from drum to drum (KICK, SNARE, HAT, CLAP, HOOVER), each in a view as the kick's (its hit large over its level, its decay marked, its memory and tune, the five drums along the bottom with their keys, the part's knobs as cards; `perc_wave`); SELECT is not the tempo there; SEL + EDIT keeps the drums' classic view | P2 | *Done; scenario drumbig.txt* |
| A-49x | **ENV RESET's list, thirteen parts** (scrolling, five in view): DROP, BUILD, KNOB, KICK, KICK TONE (the kick's EQ and FX slots), ALL KICKS (KICK 1-10 as they came), DRUMS (each drum's memory playing as it came), MIX (the MIXER's four parts), FX CHAIN (its slots, their memory forgotten), STEPS (this pattern's drum steps plain), PADS (the black keys' settings), KEYS (the white keys' loops and slice effects), TRACK (its first item, its items' breaks) | P2 | *Done; scenario kreset.txt* |
| A-49w | **Hearing the drums, and the kick reset**: stopped, on the KICK menu's classic view and the drums' pages, white keys 1–5 play the KICK, SNARE, HAT, CLAP, HOOVER (as their MIDI notes; their cards light, their key's number on them); playing, the keys play slices. ENV RESET's **KICK**: the kick selected (and its memory) back to ROTTERDAM, its SYNTH and BASS at 0, its volume kept | P2 | *Done; scenarios audition.txt, kreset.txt* |
| A-49v | **From the test firmware (test-v9.03), kept**: HOTPANT 115 out of the bank and the OVERLAY page out (the ARP black key keeps its effect); **ARP held: FILL**; a drum step's **COND** (ALWAYS, 75 %, 50 %, 25 %, 1:2, 2:2, FILL, NOT FILL, in CHANCE's bits); the kick's **BASS** part: GLIDE between notes and the REVERSE BASS (through the kick's drive and TERROR); **MB OTT** after MB COMP; ENV BUILD's **TEMPO RISE** (PRESETS, to +30 %); the visuals: a glitched start-up, the BPM lit on the beat, the header's line on each kick and the beat's tick, GLO MORE's **CRT**, the track's progress and DROP IN on the CDJ view, the kick's live scope, arcs on knob values, build and kick lights. Six setting bytes appended (kbass, drop_accel, crt) | P2 | *Done; scenario loaftest.txt, engine test test_kick_bass* |
| A-49u | **TRACKS: where PLAY starts** (SEL + PLAY on the item edited; again: the first), each track its own, saved with it (`track_t.rsv[0]`, PLAY's FROM the same); PLAY on TRACKS plays the track from there and stays on TRACKS | P2 | *Done; scenario trkfrom.txt* |
| A-49t | **MB KICK** (KICK FX's PRESETS, after MB COMP): an MB COMP preset for a gabber kick, KICK GABBER (120 Hz / 3 kHz splits; LOW −24 dB 6:1 15/120 ms, MID −20 dB 4:1 5/80 ms, HIGH −30 dB 8:1 1/50 ms; 3 dB knee; full amounts, all wet), marked in its detail's rsv[0] (no new setting); the MB COMP amount knobs' foot shows the ratio each band reaches from its detail (was 8:1 always) | P2 | *Done; scenario mbkick.txt* |
| A-49s | **Each track item its break** (TRACKS K4, the END moving to PLAY's K3 alone): SAME or a break of the bank, asked for as the item comes (`break_ask`, swapped at the bar's end, so in with the item); a DROP into the item brings that break in DROP BREAK's place. Saved in OBJ_STEP after the drum steps (`steps.tb`, 256 B; an older save loads, no breaks) | P2 | *Done; scenario trkgap.txt* |
| A-49r | **ENV held on TRACKS** (0.45 s): a space after the item edited, the items after it moved one place right, the one edited where it is (`TI_GAP`, an empty item, drawn as an outline with a +; the cursor onto it; a white key or K2 fills it, K2 left takes it out; played past if left; the track full or the cursor past the END: a message, nothing moved). ENV held on a space: it goes, the sides closed up. ENV let go before: the next part | P2 | *Done; scenario trkgap.txt* |

### 4.5 DSP effects

ABG's four, plus the master section a live unit needs.

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-40 | **Ring modulator:** frequency (tempo-synced or free, 20 Hz–2 kHz), mix | P0 | |
| A-41 | **Stereo hi-pass filter:** cutoff, resonance; stereo spread offset between L and R | P0 | No zipper noise on knob turns |
| A-42 | **Flanger:** rate (sync/free), depth, feedback (±), mix | P0 | |
| A-43 | **Degrader:** bit depth (16→2), sample-rate reduction (44.1k→2k), drive | P0 | |
| A-44 | FX page: one FX focused at a time on K1–K4; each FX on/off by SEL+K1..K4 press; fixed order ring → HPF → flanger → degrader | P0 | Total DSP < 60 % of the render budget, all FX on |
| A-45 | **Master FX chain (replaces A-44's fixed order, 1 Nov 2026):** four slots in series, each any effect of the library (FILTER LP/BP/HP, CRUNCH = A-43, DRIVE, PUNCH compressor, FLANGER = A-42, RING MOD = A-40, TREMOLO on the tempo; A-41's stereo hi-pass is FILTER in HP), three settings, mix, bypass; ALGORITHM picks the slot, PRESETS the effect, SEL + ALGORITHM moves it; per-slot state (two of one effect never share). HOME K4 was FX, the chain's dry/wet (on the MIXER since B-33) (CRUNCH moved into the chain). Settings CHP5 | P0 | *Done; engine tests: every effect alone bounded and audible, the order matters, FX 0 all dry* |
| A-47 | **DELAY** in the chain: a stereo ping-pong at 22.05 kHz on the DUB line (0.74 s per side), time on the tempo (1/16 … 1/2, 1/8T, 1/4T), feedback, tone (each repeat darker), K4 SEND (echoes return at full). One delay: also where the DUB pad and the slices' ECHO throw; the DUB pad's K1–K4 edit it when it is in the chain; without it the delay sits after the chain with the pad's settings | P1 | *Done 1 Nov 2026; engine tests: the first echo left at 3/16, the second right* |
| A-48 | **EQ** (3 bands: a low shelf at 120 Hz, a mid bell 200 Hz – 5 kHz on K4, a high shelf at 8 kHz, ±12 dB; Simper's SVF bands) and **TAPE** (drive 0–24 dB into an asymmetric soft curve, peak-matched make-up; warmth: a head bump and the top rolled off; wow and flutter on the slot's short line) in the chain | P1 | *Done 1 Nov 2026; engine tests: EQ flat, +12 dB low ×4, −12 dB high ÷4, mid ×4; TAPE harmonics on a pure tone, wow wanders the pitch* |
| A-46 | **REVERB** in the chain: a Dattorro plate (pre-delay, four input diffusers, a modulated two-half tank, fourteen output taps), at 22.05 kHz inside on 16-bit lines (~35 KB RAM; the flangers' lines shortened to 256 to pay for it); SIZE (decay), TONE (damping), PRE (0–60 ms), MIX its level over the dry; one in the chain. STOP now fades the break out (it looped on before) so tails ring alone | P1 | *Done 1 Nov 2026; engine tests: an impulse rings and dies, left and right decorrelated, a bigger size rings longer, a second REVERB passes; RAM 79 KB of 96* |
| A-49 | **MB COMP** (a 3-band compressor: splits at 200 Hz and 2.5 kHz that sum back exactly; per band an amount = threshold 0 … −30 dB and ratio 1 … 8:1 together, made up by half the reduction at full scale; attack/release per band, low slow, high fast; MIX parallel) and **CLIP** (drive 0 … +24 dB, knee soft … hard, ceiling 0 … −12 dB) in the chain. The FX page shows MB COMP's reduction per band under K1–K3 | P1 | *Done 1 Nov 2026; engine tests: the bands sum back at 0, LOW pressed: +14 dB in gives under +3.5 dB out and 1 kHz stays, CLIP clean below its edge and held at its ceiling* |
| A-49a | **MB COMP's detail** (4 Nov 2026): an MB COMP focused has six parts, FX again stepping them before the next slot (ALGORITHM still picks the slot, opening on AMOUNT): AMOUNT (each band's amount, a macro from untouched to the band's own threshold and ratio, and MIX: as A-49), LOW / MID / HIGH (THRESH −48 … 0 dB, RATIO 1:1 … 20:1 and LIMIT in 15 steps, ATTACK 0.1 – 100 ms, RELEASE 10 – 1000 ms), GAIN (each band −12 … +12 dB after its make-up, SOLO: one band alone, no dry) and X-OVER (the low split 40 – 800 Hz, the high 1 – 12 kHz, KNEE hard … 12 dB soft, OUTPUT ±12 dB). The defaults are A-49's (−30 dB, 8:1, 10/150, 5/100, 2/60 ms, 200 Hz / 2.5 kHz, hard). Its own view replaces the chain's tiles: the parts as tabs; the spectrum 20 Hz – 20 kHz in the bands' colours, filled under the response right now (each band at its reduction and gain, the make-up left out so a pressed band dips; dotted where it rests), the splits marked; per band its in/out curve with its level as a dot riding it, its level in with the threshold, its reduction with a peak held 1 s, the ratio, threshold and gain as they act; ~30 frames a second. `brk_mbc_t` per slot (`brk_fx_set_mbc`, `brk_fx_mb_meter`), appended to the settings (320 B, moves with its slot) | P1 | *Done; engine test test_mb_detail (1:1 leaves the band, LIMIT presses harder than 8:1, GAIN +12 dB ×4, OUTPUT −12 dB ÷4, SOLO LOW / HIGH, the low split moved takes 500 Hz, ATTACK and RELEASE, the knee under the threshold); scenario mbcomp.txt; +5.7 KB flash (576.9 KB), +0.8 KB RAM* |
| A-49b | **Every effect's view, MORE and MOD** (4 Nov 2026): each effect but MB COMP has three parts on FX again (then the next slot; ALGORITHM picks the slot): MAIN (as A-45), MORE (`brk_fxx_t` x[0..3]: FILTER drive, 12/24 dB, stereo spread; CRUNCH tone, dither; DRIVE shape SOFT/TUBE/HARD/FOLD, bias, low cut; PUNCH ratio 1.5–20:1, make-up, side-chain HPF; FLANGER start delay, spread, tempo sync; RING MOD wave, RING/AM; TREMOLO shape, smooth; REVERB width, low cut, mod, diffusion; DELAY width, loop low cut, duck; EQ low and high shelf frequencies, mid Q, level; TAPE flutter, wow rate, hiss; CLIP bias, output), every default the effect as it was; MOD (x[4..7]: rate on the tempo 4 bars … 1/16 and triplets, bipolar depth, shape SINE/TRI/SAW/RAMP/SQUARE/S&H/ENV (the slot's own input followed), target K1/K2/K3/MIX; applied once a block to the render's copy of the slot). The extras carry the effect they are for: another effect's are not used (its defaults are), so old saves and moved slots stay right. Its view replaces the slot tiles: the parts; a picture computed from the settings as modulated now (FILTER and FLANGER responses, EQ ±12 dB, a tone through DRIVE / CLIP / TAPE / CRUNCH / PUNCH / RING / TREMOLO, REVERB's decay in dB, DELAY's echoes); the chain as chips; the modulator's cycle with its dot and target; the slot's level in and out (`brk_fx_meter`). Appended to the settings (`fxx`, 36 B a slot, moves with its slot). Flash: the engine's block-rate code is `BRK_COLD` (minsize) to pay for it | P1 | *Done; engine test test_fx_extras (every effect × every extra at both ends × every modulator bounded; FILTER 24 dB, SPREAD; EQ LOW F, LEVEL; CLIP OUTPUT; REVERB and DELAY WIDTH 0; PUNCH RATIO; MOD sweeping a cutoff; ENV); scenario fxviews.txt; image 581,036 B of 581,564* |
| A-45 | **Master bus:** DJ filter (one knob LP↔HP), compressor with drive, brickwall limiter | P1 | No output clipping at max settings |
| A-46 | Send FX: tape/dub delay (tempo-synced, feedback, filter in loop) and small reverb | P1 | |

### 4.6 Recording and export

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-50 | Record the performance: the FM-1 is a **USB audio input**, so any DAW or recorder captures the master output | P0 | Works class-compliant on macOS, Windows, iOS |
| A-51 | Recording starts in sync with the loop: a USB MIDI Start goes out on PLAY (DAWs can arm on Start) | P1 | |
| A-52 | **Resample:** bounce 1–2 bars of the master output into a new break slot (flash) | P2 | Bounce encodes to ADPCM in < 3 s |

### 4.7 Custom loops

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-60 | ~~Import up to **8 custom loops**~~ **Dropped 3 Nov 2026:** CHOPPAH does not load samples of its own; the breaks are the firmware's (`breaks/breaks.txt`, A-61). A-62–A-64 go with it | — | |
| A-61 | **Prep tool** (`tools/prep_breaks.py`, Python stdlib only): WAV in → trim to whole bars (by BPM or detected) → loudness to −16 dBFS RMS of the loud parts with a look-ahead peak limiter (3 Nov 2026; was peak-normalised, THINK sat ~8 dB under DRUMMER) → slice markers (grid or transient) → resample → IMA ADPCM → bank image + manifest | P0 | Amen 2 bars fits in < 45 KB; the 8 breaks within 0.6 dB of each other at the output (simulator) |
| A-62 | **Upload** over USB: CLI (`tools/fm1_breaks.py upload`) and a web page (Chrome/Edge, Web MIDI SysEx) | P0 | A full bank uploads in < 60 s; a failed upload leaves the old bank intact (A/B or verify-then-commit) |
| A-63 | On-device slice editor (EDIT): move markers, nudge in 1 ms, set slice count, set native BPM, rename | P1 | |
| A-64 | Bank capacity shown before upload ("what fits"), including `--dry-run` without the FM-1 | P0 | |

### 4.8 MIDI control

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-70 | Transport: Start/Stop/Continue in; **MIDI clock in** (USB and TRS) with tempo follow; clock out over USB | P0 | Locks to CDJ/DAW clock in < 2 beats; follows tempo changes. *Done 30 Oct 2026 on the GLOBAL page: clock in follows the tempo (a beat of ticks, smoothed), START/STOP/CONTINUE (continue starts again: no song position yet), phase eased every beat (`brk_sync`, jumps when > 1.5 steps off); clock out 24 ppqn sample-marked by the engine, sent on the 1 kHz tick, START/STOP with PLAY; MIDI TO picks USB / Bluetooth / both. Engine tests: 96 ticks a bar with and without swing, easing onto a clock 0.4 step off, jumping. Host scenario midi.txt* |
| A-71 | **Bluetooth MIDI** (fm1-base's own stack): on/off on the GLOBAL page, at power-on as saved, advertised as "CHOPPAH XXXX"; clock in and out over it | P1 | *Done 30 Oct 2026 (~40 KB of flash)* |
| A-72 | **Bluetooth audio out** (fm1-base's A2DP source, SBC, `bt_audio = true`): GLOBAL's third part: K1 on / off (at power-on as saved, the last device connected again), K2 the device (remembered, then found), K3 a 10 s scan, K4 connect / hang up (SEL + left: forget). The FM-1's own output mutes while it streams; stereo; K3 while streaming plays a left / right test (a 440 Hz beep left, 880 Hz right, twice: 3 Nov 2026, a user heard only the right side; the SBC encoder decodes both channels on a computer, with gcc, clang and libavcodec's decoder, and so does JieLi's clang 4.0.1's optimised IR for the FM-1, run on x86); K2 while streaming picks the SBC coding, JOINT (fm1-base's) or plain STEREO (the encoder set again in the same unit, its bitpool kept): the same user still heard only the right side; 150–300 ms late (listening, not playing). Paid for with the 31-pixel font (20 KB of flash: the title is FONT_B doubled, the platform's big face FONT_B), the bank build's synthesised-break code, the canvas cut to 240 × 60 (the pad and key views drawn in two bands: 17 KB of pool for the packet queue) and the DUB line to 0.70 s | P2 | *Built 2 Nov 2026; host scenario btaudio.txt with a pretend radio (the host has none); not yet heard from an FM-1* |
| A-75 | **Notes play slices and drums** (3 Nov 2026): notes 48–63 (C3–D#4) on any channel play slices 1–16, held like the white keys (snap, key loops); 36 the kick, 38 the snare, 42 / 44 / 46 the hat, also while stopped; with REC on and playing they record into the pattern (B-22) | P0 | *Done; scenario seq.txt: a note recorded, a slice played by note* |
| A-72 | Break select by notes (separate octave) and Program Change; pattern select by Program Change | P1 | |
| A-73 | CC map for K1–K4 and all FX parameters; CC out on knob turns | P1 | Map documented in the manual |
| A-74 | MIDI channel selectable (GLO) | P0 | |

### 4.9 Help and interface

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| A-80 | Hold-for-help cards on every button | P1 | |
| A-81 | Manual (docs/MANUAL.md → site) | P1 | |
| A-82 | Screen-reader words (fm1-reader) | P2 | |

---

## 5. Feature set B: Performance mode

Performance mode is the default **HOME** screen with the keys set for playing live. The aim: two hands
on the keys, eyes on the crowd.

### 5.1 Black-key FX pads (momentary, combinable)

| Key | Pad | What it does while held | Pri |
| --- | --- | --- | --- |
| B1 | **ROLL 1/8** | Beat-repeat the current position at 1/8 | P0 |
| B2 | **ROLL 1/16** | ... at 1/16 | P0 |
| B3 | **ROLL 1/32** | ... at 1/32 (with B1/B2: 1/8T, 1/16T) | P0 |
| B4 | **REVERSE** | Reverse gate (A-35) | P0 |
| B5 | **FREEZE** | A-30 | P0 |
| B6 | **HALF** | Half-speed (one octave down, tape style) | P0 |
| B7 | **TAPE STOP** | Pitch-and-speed ramp down over 1/4–2 beats; release spins back up | P0 |
| B8 | **STUTTER GATE** | Trance gate at 1/16 with swing | P1 |
| B9 | **RANDOM** | Step randomisation (A-31) | P0 |
| B10 | **FILTER THROW** | Sweeps the DJ filter to LP (SEL: HP) and back on release | P1 |
| B11 | **DUB THROW** | Sends the current step into the dub delay at 100 % | P1 |

- Pads combine (ROLL + REVERSE = reversed roll).
- **While a pad is held, the screen shows its animation and K1–K4 tune it** (built 30 Oct 2026): rolls rate /
  decay / pitch ramp, reverse speed, freeze length / pitch, half speed, tape stop and spin-up times, gate rate /
  length, random amount / pitch, filter target / resonance / speed, dub time / feedback / tone / mix. Kept by SAVE.
- B11 DUB THROW is a tempo-synced dub delay (64 KB line in the pool) whose echoes ring on after the pad.
- Pad assignments are configurable in GLO (P2).
- **SEL + black key** latches a pad until pressed again (P1).

### 5.2 Live chopping

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| B-10 | **Hold a slice key = loop it** at the step grid until released; release rejoins in phase | P0 | |
| B-10a | **Key loops (done 30 Oct 2026):** each white key has its own loop, tuned with K1–K4 while held: LENGTH 1/8–16 slices, PITCH ±12, START 0–15/16 of the slice, MODE loop / ping-pong / reverse / one shot; SEL resets. Held ≥160 ms (or a knob turned) the strip shows the key's window zoomed, the played part lit, the playhead, repeat dots; a dot marks keys with settings. Works stopped too. Saved with SAVE | P0 | The loop restarts on the tempo grid (engine test); one shot turned back to loop loops again |
| B-11 | **Two slice keys held = ping-pong** between them each step | P1 | |
| B-12 | **Scratch scrub:** turning SELECT on HOME scrubs the playhead (vinyl-style), with momentum; release returns to the loop | P1 | Scrub audio is continuous (no gaps) |
| B-13 | **Choke / mute group:** slices cut each other (mono) or overlap (poly, 4 voices) | P0 | Default mono, like a classic sampler |
| B-14 | **Swing** 50–75 % on the 1/16 grid | P0 | |
| B-15 | **Double-time / half-time** of the sequencer (not the pitch) | P1 | |
| B-16 | **Slice pitch on the fly:** hold a slice key and turn K1 to repitch that slice ±12 | P1 | |

### 5.3 Step sequencer with locks

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| B-20 | **16 patterns** (3 Nov 2026; LOAF BREAKER: 64, A-51e), each the break's steps (32: the breaks are 2 bars; not changed to 1.5 bars) and three drum lanes of 16 steps (kick, snare, hat) repeating every bar. PRESETS picks one on most pages; while playing the change waits for the loop's end (the header shows "P2>"). Saved in OBJ_PAT (0xDA000, 1.6 KB) with SAVE | P0 | *Done; scenario seq.txt* |
| B-21 | Per step: slice (LOOP = follow the break, where the generator works, a slice of its own, or OFF), pitch (±12), ratchet (HITS 1–4), reverse. SEQ page: ALGORITHM picks the lane (STEPS 1-16, STEPS 17-32, KICK, SNARE, HAT); on a break lane the white keys pick the step and K1–K4 set it, on a drum lane they toggle its steps. Gate length, probability and FX locks per step not built | P0 | *Done (partly: no per-step gate, probability or FX lock)* |
| B-22 | **Record live:** REC (any page) while playing writes each white key into the pattern at the step it lands on (the step playing, or the next past its middle), MIDI notes too (A-75); SEL + REC clears the lane on SEQ, the pattern elsewhere | P0 | *Done; scenario seq.txt* |
| B-23 | Pattern change at the loop's end (built, B-20); chain patterns (song mode, up to 32 entries) | P1 | |
| B-24 | Copy/paste step, page, pattern (SEL+SAVE / SEL+key) | P1 | |
| B-25 | Knob motion recording (one lane per FX param, P-lock style) | P2 | |
| B-30 | **Gate** (PERFORM K3, K1 since B-33, global): every slice the loop plays sounds for 5–100 % of its own length, then a 3 ms fade; follows tempo and pitch; for the loop, ratchets and RANDOM (key loops and rolls have their own lengths; FREEZE ungates). Replaced the PERFORM page's copy of CMPLX (still on GENERATOR). *Done 31 Oct 2026; engine test: at 25 % the second half of each slice is silent* | P1 | |
| B-31 | **Snap** (GLOBAL, second half, K3): where a white key pressed while playing lands: OFF (at once) or the next point of a grid 1/8, 1/4, 1/2 (default: the old next-16th on 2-bar breaks), 1, 2 or 4 slices apart from the pattern's start; whole steps follow the swing (on_step), less than a step is counted in samples. *Done 1 Nov 2026; engine tests: 2 slices waits a step more, OFF at once, 1/4 slice a quarter step on* | P1 | |
| B-33 | **MIXER on LFO** (2 Nov 2026; 3 Nov: the soundboard's OVERLAY gave way to PERC): LEVELS: K1 BREAK (the break and its throws, eased), K2 KICK (= kick VOLUME), K3 PERC (the snare and the hat), K4 FX (the chain's dry/wet), as faders with the break's, the kick's and the drums' meters and the chain's slots; LFO again, GROOVE: K1 swing, K2 DRUMS, the drums' route. Swing and FX moved off PERFORM, which keeps GATE (K1) and the DJ filter (K2). BREAK level appended to the settings | P1 | *Done; scenario mixer.txt: the break down leaves the kick alone, both down is silence* |
| B-34 | **The DJ filter is the break's** (2 Nov 2026): it moved from the end of the output stage to the break's input of the chain (the break and its echo / reverb throws); the drums through the chain join after it, the drums around the chain later still, so a sweep leaves them full. The FLT pad throws it the same way | P1 | *Done; engine test: low-passed, a 4 kHz break is gone while the kick passes* |
| B-32 | **Slice effects:** each slice its own effect while it plays (loop, key, roll): LO-PASS, HI-PASS, CRUNCH, DRIVE, RING MOD, ECHO / VERB throws into the chain's delay and reverb, plus its LEVEL. A key held + FX flips K1–K4 to it (EFFECT, AMOUNT, TONE, LEVEL); blocks show a tag. Run per voice (a slice's state starts with its hit), the throws on two send buses out of the engine. Settings CHP6 | P1 | *Done 1 Nov 2026; engine tests: level 0 silences that slice alone, LO-PASS darkens it, ECHO throws it alone* |

### 5.3a Control rules (3 Nov 2026, from the control map's review)

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| B-40 | **Encoder roles:** ALGORITHM picks what the page edits (FX slot, kick layer, SEQ lane, drum voice, GLOBAL's part), the break on HOME / ARP / MIXER; PRESETS what goes in (effect, kick, drum sound), the pattern elsewhere. SEL + ALGORITHM = the break and SEL + PRESETS = the pattern on every page | P1 | *Done; scenario controls.txt* |
| B-41 | **A page button again = its next part**: GLO 3 parts, EDIT 5, LFO 2, ENV 2, FX its 4 slots (on letting go, so FX held + ALGORITHM can move the slot), SEQ its 5 lanes; a dot per part between the pads and the knobs. FX's bypass moved to SEL + FX | P1 | *Done; scenarios fxchain.txt, controls.txt* |
| B-42 | **Undo:** one copy of the pattern before SEL + ARP, SEL + REC and a drum lane's FILL / CLR / COPY (a burst of one change is one); SEL + SAVE swaps it back (again: redo); ~140 B of RAM | P1 | *Done; scenario controls.txt* |
| B-43 | ~~**The page's help line** under the waveform: KEYS / ALGO / PRESETS~~ Removed 3 Nov 2026 (the user's call: the tabs say enough; the waveform has its rows back, 44) | P2 | *Removed* |
| B-44 | **The drums' route has one home**, MIXER GROOVE K2: SEL + EDIT and KICK OUT's ROUTE removed | P2 | *Done* |
| B-45 | **HOME K3 FX, K4 COMPLEXITY** (mirrors of MIXER K4 and the GENERATOR's K1) | P2 | *Done* |
| B-46 | **SEQ drum lanes' knobs:** K1 FILL (n hits spread evenly from the downbeat, Euclidean), K2 ROTATE, K3 clear (left) / four on the floor (right), K4 copy the previous pattern's lane | P2 | *Done; scenario controls.txt* |
| B-47 | **SEL pressed alone** (held a second until 3 Nov 2026, when tap tempo left SEL): the strip lists the SEL combinations (FX page: SEL + FX, FX + ALGO); SEL again, or any other button, key or knob, closes it | P2 | *Done; scenario controls.txt* |
| B-50 | **EDIT's drums show their waveforms** (3 Nov 2026): the lower strip's three boxes each draw one hit of the drum (`kick_shape`, `perc_shape`: offline, worked out again when its settings change), over a window fitting its decay, as tall as the box | P2 | *Done; engine test: perc_shape a hit, then its tail* |
| B-51 | ~~**Fills on ARP**~~ (3 Nov 2026, v0.52): eight fills on the white keys (ROLL, SNARE / KICK rushes, RANDOM, REVERSE, STOP, DROP, HALF), an auto fill. **Dropped** for B-52 the same day: the engine's fills removed, their 8 settings bytes kept as reserved | — | *Replaced by B-52* |
| B-52 | **Overlay effects on ARP** (3 Nov 2026): sixteen effects over the whole mix (after the FX chain), one a white key, on while held, any together, each fading in and out over 5 ms: DIVE (low-pass sweeping down), RISE (high-pass sweeping up), RADIO (band-pass), WAH (resonant band on the beat), CRUSH (bits), LO-FI (sample rate), DIST, RING (ring mod), CHOP / GATE (level cut on the rate, twice it), DUCK (pump each beat), PAN (auto-pan), BOOM (bass), WOBBLE (tape wow / flutter), JET (flanger), CUT (silence). Each its own AMOUNT (K1: the one touched last); K2 RATE 1/4–1/32, K3 SWEEP 1/4 bar–2 bars, K4 LATCH (press on / press off, latched ones stay on other pages). `dsp/ovl.c` (platform-free), a 4 KB delay line in the pool, settings appended after the reserved bytes | P1 | *Done; engine test test_ovl; scenario overlay.txt* |
| B-53 | **The start screen lights the buttons** (3 Nov 2026): a comet runs through the 14 buttons as the break drops in (OCT− and OCT+, the top row left to right, the bottom row back; one lit, two mid behind, the rest it passed glowing), all flash as the mark completes, then PLAY blinks green with PRESS PLAY; normal lights once the slicer opens | P2 | *Done* |
| B-54 | **Demo mode** (3 Nov 2026): untouched for a minute while stopped (three while playing), five full-screen scenes of 8 s at 25 fps, on the tempo (the break's beat while it plays): BARS (the mark bouncing to an Amen beat), WAVE (the waveform scrolling, a slice chopped each beat), TUNNEL, WORMHOLE (a wireframe tube of twisting rings bending to a glowing core, a lurch each beat; added the same day), STARS; the buttons' comet round and round, the keys with each scene (the bars, the playhead, the tunnel outwards, the wormhole's swirl, twinkling). Any key, button or knob ends it and does nothing else. `ui/demo.c`, ~0.9 KB of RAM (the stars in the pool) | P2 | *Done; scenario demo.txt* |
| B-48 | **SEQ sequences, EDIT designs** (3 Nov 2026, the user's call): the drums' steps live on SEQ's lanes only (the white keys play slices on every other page); EDIT holds the kick, the snare and the hat (EDIT again: the next drum; ALGORITHM the drum's part: the kick's 11 = MIX, each layer and its FX, MACRO, OUT; the snare's / hat's SOUND, SHAPE); the lower strip shows the three drums, lit on their hits. ENV went to tap tempo (A-38) | P1 | *Done; scenarios kick.txt, perc.txt* |
| B-49 | **A tab bar** replaces B-41's dots: right under the header on every page (since 3 Nov 2026; first under the waveform), tap tempo's too, the button's name as printed, then the page's parts as tabs (the one you are on filled amber), and on EDIT / FX the part edited, "< SUB FX >"; the help line (B-43) under it | P1 | *Done* |

### 5.4 Performance screen (HOME)

- Top bar: break name · BPM (tap to set) · bar.beat · pattern · clock source icon.
- Middle: waveform of the break with slice markers, the playing slice highlighted, playhead.
- Pad strip: 11 small boxes lit when a black-key pad is active.
- Bottom: K1–K4 labels and values: default **FILTER · PITCH · COMPLEXITY · FX MIX**.
- Key LEDs: white key of the playing slice lit; held pads lit; steps lit in SEQ.

---

## 6. Modes and pages

| Page (button) | K1 | K2 | K3 | K4 | SEL page |
| --- | --- | --- | --- | --- | --- |
| HOME (perform) | Gate | DJ filter on the break | FX | Complexity | Tempo (tap) |
| LFO (mixer, built) | Break level | Kick level | Snare and hat level | FX | LFO again: swing · drums route |
| SEQ (built) | Step slice | Step pitch | Hits (ratchet) | Reverse | ALGORITHM: the lane |
| HOME again (generator) | Complexity | Anchor | Roll | Fill | Retrig 2× · 3× · 4× · 8× |
| ARP (overlay, built) | Amount (the effect touched last) | Rate | Sweep | Latch | White keys: the 16 effects (B-52) |
| EDIT (break) | Slice select | Start nudge | Slice count | Native BPM | Gain · Trim · Name · Mode (repitch/stretch) |
| FX | Focused FX param 1 | param 2 | param 3 | Mix | FX again: MORE, MOD (an MB COMP its six parts), then the next slot · SEL + FX on/off |
| ENV (snare / hat, built) | Level | Tune | Decay | Tone | ENV again: snap / cut · drive |
| LFO (planned; the button is the MIXER now: an LFO needs another home) | Rate (sync) | Depth | Target | Shape | Retrigger on bar |
| GLO | MIDI channel | Clock source | Clock out | Looper length | Pad map · Velocity · Brightness |

---

## 7. Sound engine requirements

| ID | Requirement | Pri |
| --- | --- | --- |
| E-1 | Playback interpolation: 4-point Hermite (P0); linear as a "crunch" option (P1) | P0 |
| E-2 | Voices: 1 mono (default) or up to 4 poly with choke groups | P0 |
| E-3 | Sample-accurate step scheduling inside the 256-frame block (no 1 ms tick jitter) | P0 |
| E-4 | Fades on every start/stop/cut (A-6) | P0 |
| E-5 | Time-stretch mode (granular, tempo without pitch change) per break | P2 |
| E-6 | Character: optional 12-bit / 26 kHz "SP-1200" output stage, separate from the Degrader | P2 |
| E-7 | Overload guard: if the render exceeds 85 %, drop poly voices first, then FX quality, never audio (`APP_OVERLOAD`). *Done 4 Nov 2026 (v0.60 glitched and reset under heavy chains: the watchdog, the render starving the loop): the platform calls `app_overload` over 90 %; the FX chain bypasses its last slot on (one more each ~0.2 s it goes on), each back after 2 s clear, the header says OVERLOAD (`brk_fx_overload`, `brk_fx_shed`); engine test test_overload. The chain's per-sample loop now runs from RAM (`fx_samples` in .ram_text, BRK_RAM: no flash-cache misses, stored LZMA-packed), the meters follow only the slot on screen (or one whose ENV needs them)* | P0 LOAF BREAKER 0.219 (FLEXOR: OVERLOAD shown now and then): a lone block over 90 % sheds nothing, a second within 32 blocks (0.19 s) does; MB COMP's gains every 16th sample, ramped (about a fifth of its cost off) |
| E-8 | FX chain can run on the second core (`plat_cpu2_run`) with fallback to one core | P1 |
| E-9 | **Pitch mode** (GLOBAL, second half): KEEP LENGTH (the default since 1 Nov 2026) or SPEED (repitch); KEEP LENGTH is a granular pitch shift in the voices (two triangle grains of 10–80 ms reading at the pitch while the voice runs at the tempo's speed) for every semitone pitch; speed effects stay speed. *Done 31 Oct 2026; engine test: +12 on a 1 kHz tone sounds at 2 kHz with the voice at 1x* | P1 |
| E-10 | **Roll level and swell:** the rolls' K4 LEVEL (5–100 %, the repeats' start level: ghost notes) and DECAY both ways (down: each repeat quieter; up: a swell to full). *Done 31 Oct 2026; engine tests* | P1 |

---

## 8. Architecture

### 8.1 Code layout

```
app/
  app.h, app.c      platform contract (init, render, panic, store map), settings, patterns, MIDI, the render
  ui/*.c            the pages, included by app.c in order (one unity build, 3 Nov 2026): common, fx, pads,
                    keys, bta, global, seq, kick, perc, mixer, screen, main (input, LEDs, the frame)
  dsp/
    brk_bank.[ch]   sample bank format: header, slots, slice tables, IMA ADPCM data in flash
    adpcm.[ch]      IMA ADPCM decoder (into pool RAM)
    brk_voice.[ch]  slice voices: Hermite resampler, fades, reverse, pitch
    brk_seq.[ch]    clock, steps, quantise, swing, looper, pattern chain
    brk_gen.[ch]    the generator (complexity, anchor, roll, fill, retrig)
    brk_fx.[ch]     ring mod, stereo HPF, flanger, degrader, DJ filter, comp/limiter, delay
    kick.[ch]       the four-layer kick (§11b)
    perc.[ch]       the snare and the hat (§11b, K-10)
tools/
  prep_breaks.py    WAV → slices → ADPCM → bank image
  fm1_breaks.py     upload / dump / dry-run over USB-MIDI
tests/
  engine/           host unit tests for dsp/ (plain gcc, no hardware)
  scenarios/        fm1-base host-simulator scripts (keys in → audio + screenshots out)
```

The engine in `app/dsp/` has **no platform dependencies**, so it builds and is tested with plain `gcc`
on any computer, and in the browser emulator.

### 8.2 Memory plan

| Region | Use | Budget |
| --- | --- | --- |
| Flash: app slot (581 KB) | code, UI assets, the built-in breaks | ≤ 450 KB planned; 3 Nov 2026: 552.6 KB with Bluetooth MIDI and audio and 8 breaks (577.9 before: fm1-base updated to 2d8ed74 with `unity_opt = "z"` and `boot_pack`, the platform screens drawn in CHOPPAH's faces (`APP_SYS_TEXT`), the soundboard out) |
| Flash: 0x97000–0xD9FFF + XDATA1 0xE5000–0xFBFFF | break bank (8 slots) | ≤ 356 KB |
| Flash: 0xDA000, 0xDC000, 0xFC000 | store objects: patterns, panel calibration, settings | 2 sectors each |
| Pool RAM (336 KB) | screen canvas 240 × 96 (45 KB, drawn in strips); **active break decoded to int16 PCM**: 2 × 112 KB (current + next/B-loop, so switching is instant); FX delay line 40 KB; scratch 8 KB | ≤ 320 KB |
| `.data/.bss` (96 KB) | engine state, patterns in RAM, UI | ≤ 64 KB |

**Why decode to RAM:** ADPCM can't be read backwards or from any sample, while scrub, reverse, roll and
freeze all need random access. A 112 KB buffer holds 2.6 s of 22.05 kHz mono (1.5 bars of the Amen at
its native tempo) or 1.3 s at 44.1 kHz; a 2-bar break at 22.05 kHz needs 154 KB, so long breaks take both
buffers (no instant switching for those). M1 uses one 128 KB buffer. The prep tool chooses the rate per break to fit; 22.05 kHz is the
default (it's also the jungle sound).

**Flash budget example:** IMA ADPCM at 22.05 kHz = 11 KB/s → 360 KB ≈ 33 s of breaks ≈ 8 × 2-bar breaks.

### 8.3 Bank format (v1)

Defined in `app/dsp/brk_bank.h` and written by `tools/prep_breaks.py`: a 16-byte header (`CHB1`, version,
slot count, data size, CRC-32 of everything after it), a slot table of 168-byte entries (name, BPM ×100,
rate, length, data offset and size, bars, slice count, 33 slice offsets), then each slot's IMA ADPCM. A break
decodes whole into RAM from its start, so no per-slice decoder state is stored. Written verify-then-commit:
data first, header last, so a cut upload leaves the old header valid or nothing (A-62).

### 8.4 Persistence (store objects)

| Object | Content | Size |
| --- | --- | --- |
| OBJ_SETTINGS (0xFC000) | globals, MIDI, last break/pattern/tempo, FX state, the drums. Magic CHP7 (3 Nov 2026: the soundboard's and the kick's steps out, the snare and hat in, so older saves start at the defaults once); the struct only grows at the end and a shorter save of CHP7 loads into the front (the rest keeps its defaults) | < 1 KB |
| OBJ_PAT (0xDA000) | 16 patterns × (32 steps × 4 bytes + 3 lanes) | ~1.6 KB (fits 3840 B max) |
| OBJ_SONG | chain | < 256 B |
| OBJ_PANEL (0xDC000) | panel calibration (platform's) | |
| OBJ_BLE (0xDE000) | Bluetooth pairings (platform's); RESET ALL keeps them | |

### 8.5 Timing

- Clock: internal BPM or MIDI clock (24 ppqn), smoothed; position in samples (double-free: 32.32 fixed point).
- Each render block computes the step boundaries that fall inside it and starts voices at the exact frame.
- UI runs in `app_frame` (~60 Hz); UI → engine through a lock-free event queue; engine → UI through a
  snapshot struct (playhead, slice, levels).

---

## 9. Non-functional requirements

| ID | Requirement | Target |
| --- | --- | --- |
| N-1 | No audio dropouts | 0 xruns in a 1-hour soak with all FX and 4 voices (`plat_xruns`) |
| N-2 | Render load | < 70 % worst case on one core (`plat_cpu_pct`) |
| N-3 | Latency key → sound | < 10 ms free-play; quantised: next step |
| N-4 | Boot to playable | < 2 s |
| N-5 | Flash writes never interrupt playback unexpectedly | saves only on SAVE / stop; show a "saving" note (writes stall audio briefly) |
| N-6 | Tests | engine unit tests + host scenarios run on every change (CI on GitHub Actions) |
| N-7 | Licence | MPL-2.0 (pending §11 Q3) |
| N-8 | Radio | never enabled (`ble = false`) |

---

## 10. Install, update and recovery

- **Install:** `python3 base/tools/fm1_install.py app/build/choppah.fwsc` over USB-MIDI, or the generated
  web installer page (Chrome/Edge). Update mode on the FM-1: hold **OCT− + OCT+ for 5 s**.
- **Rollback:** the web installer can put M-VAVE's own firmware back. Keep a stock firmware file locally
  before the first flash.
- **The update screen** (3 Nov 2026): while the package is written the FM-1 shows CHOPPAH's own: black, the slice
  mark filling in amber with the write, UPDATING, DO NOT UNPLUG, a bar of amber segments. It is the update loader's
  screen (felucca-bsp `loader/ldr_screen.c`, no hook): `build.sh` swaps in `app/loader/ldr_screen.c` for device builds
  and restores felucca-bsp's after; only the drawing differs (the panel's start, the functions, RAM-only buffers as
  the original). The loader ships in the package, so the screen shows when that package is installed.
- **Safe mode:** fm1-base starts in safe mode after two failed boots, and the installer can still reach it.
- **Last resort:** if the chip's own update mode shows up (USB 4C4A:8057), `base/bsp/tools/fm1_rescue.sh`
  restores stock firmware from a Mac.
- The single-bank flash has no proven recovery for an interrupted write: never unplug during an update,
  and flash with a charged battery.

---

## 11. Open questions

| # | Question | Default if not decided |
| --- | --- | --- |
| Q1 | **Built-in breaks.** The Amen and other recorded breaks are copyrighted recordings; X0X ships none. Ship user-loaded only, plus a synthesised "Amen-like" break played by an internal kit? | **Decided 30 Oct 2026:** five synthesised styles plus 12 of Cyberworm's royalty-free Amens. **Changed 1 Nov 2026 (Laurens):** the synthesised styles out (kept only as the fallback of a build without a bank), and 8 classic breaks from the records, 2 bars at 22.05 kHz (360 KB): Amen, Funky Drummer, The Worm, Think, Cold Sweat, Hot Pants, Apache, Sport; Rotary Connection and Skull Snaps left out for room. Commercial recordings, published by the owner's decision. The PCM buffer grew to 106,496 samples for the slow ones |
| Q2 | Name: CHOPPAH / BREAK CHOPPAH / AMEN-1? | CHOPPAH |
| Q3 | Licence: MPL-2.0 (like fm1-base) or GPL-3.0 (needed if we port GPL code such as X0X's BB Gen port)? | MPL-2.0, write the generator from scratch |
| Q4 | Default sample rate 22.05 kHz (more breaks, crunchier) or 32 kHz (cleaner)? | 22.05 kHz, per-break override |
| Q5 | Package identity `id_prefix`: needs a unique FM-1_ number | LOAF BREAKER: `FM-1_2623` (9 Nov 2026, free on the Firmware Hub's catalogue; CHOPPAH keeps `FM-1_86`); files `loaf-breaker*` (fm1.toml `name`); docs/hub/SUBMIT.md |
| Q6 | Theme and splash (e.g. Disco in Paradise branding)? | Black / amber |

---

## 11a. Soundboard (built 30 Oct 2026, removed 3 Nov 2026)

ENV was a soundboard of 16 synthesised sound-system effects (air horn, siren, rewind…) over the break. It was taken
out on 3 Nov 2026 with the REWIND pull-up, for the pattern sequencer and the snare and hat (ENV now opens SNARE / HAT).

## 11b. Drums: kick (built 1 Nov 2026), snare and hat (3 Nov 2026)

A synthesised kick over the break, from the Gabber Kick Designer brief: four layers summed into a drive and a
clipper, on the pattern's kick lane (B-20), through the FX chain or around it. `app/dsp/kick.c` (platform-free, its
own `o2` unit); settings in OBJ_SETTINGS (136 B).

| ID | Requirement | Pri | Acceptance |
| --- | --- | --- | --- |
| K-1 | **Four layers:** SUB (a sine dropping from 3× to 30–90 Hz, saturated by TONE, 60–1500 ms), PUNCH (a sine swept down from up to 18×, 50–200 Hz, 20–400 ms), GRIT (FM at 1.41× into a drive, noise by TONE, 40–400 Hz, 20–800 ms), CLICK (high-passed noise against a ringing click, 1–12 kHz, 1–40 ms); each LEVEL, PITCH, DECAY (to −40 dB), TONE | P0 | *Done; engine tests: each layer alone sounds, SUB's pitch within 10 %, DECAY lengthens the tail* |
| K-2 | **Layer FX:** per layer OFF / DRIVE (gain, tone) / CRUNCH (bits 15–3, rate) / FILTER (low-pass 60 Hz–11.6 kHz, resonance) / RING MOD (30 Hz–1.9 kHz) with WET | P0 | *Done; every layer through every effect bounded* |
| K-3 | **INTENSITY:** the sum's drive (0.3–30×) into a soft clip, turning hard over half (the distorted gabber kick), made up to full scale; VOLUME | P0 | *Done; INTENSITY raises the body* |
| K-4 | **KICK on EDIT** (since B-48 ALGORITHM turns its parts: MIX, each layer, its FX, MACRO, OUT; before, EDIT again stepped MIX (K1–K4 the layers' levels) → LAYER (LEVEL PITCH DECAY TONE) → LAYER FX (EFFECT, A, B, WET) → MACRO (SUB, PUNCH, GRIT, INTENSITY) → OUT (VOLUME, ROUTE); ALGORITHM the layer; the strip shows the kick's shape (600 ms), the layers' levels, the 16 steps with the playhead | P0 | *Done; simulator scenario kick.txt* |
| K-5 | **Steps** (the pattern's kick lane since 3 Nov 2026): on the KICK page the white keys toggle the kick on the bar's 16 steps (empty by default since 3 Nov 2026, was four to the floor; LEDs show them, the step playing flipped). The engine marks each step's sample offset (`brk_step_events`) so the kick lands sample-accurate, swing included | P0 | *Done; engine test: 16 events a bar, a step apart, places 0–15* |
| K-6 | **Presets:** PRESETS loads a built-in kick (909, TECHNO, EARLY HC, ROTTERDAM the default, INDUSTRIAL, DISTORTO); the steps, routing and volume stay; SAVE keeps the kick with the settings | P0 | *Done; six presets in range and distinct* |
| K-7 | **Routing** (the kick, the snare and the hat together since 3 Nov 2026): through the FX chain with the break (default) or around it (joins after the chain, into the limiter, thrown nowhere); never through the break's DJ filter (B-34); MIXER GROOVE K2 (B-44; was also SEL + EDIT and OUT K2) | P1 | *Done* |
| K-8 | **MIDI:** note 36 (C1) note-on on any channel plays the kick at the next block, also while stopped | P1 | *Done; scenario: a note sounds while stopped* |
| K-9 | Retrigger without clicks: the kick still sounding fades out under the new hit; no work while silent | P0 | *Done; engine test* |
| K-10 | **SNARE / HAT on EDIT** (on ENV until B-48; `app/dsp/perc.c`): SNARE two sines (1 : 1.62) with a pitch drop, band-passed noise and a crack (LEVEL, TUNE 120–400 Hz, DECAY 60–700 ms, TONE, SNAP, DRIVE); HAT six squares at the 808's ratios plus noise through two high-passes (LEVEL, TUNE, DECAY 15–900 ms, METAL / NOISE, CUT, DRIVE); four presets each (909, TIGHT, FAT, GABBER; CLOSED, 808, OPEN, TRASH); ALGORITHM the voice, ENV again SOUND / SHAPE, the white keys its lane; MIXER K3 PERC; MIDI 38, 42 / 44 / 46 | P1 | *Done; engine tests: presets bounded, the snare's body at its tune; scenario perc.txt* |
| K-11 | **RESET ALL** (GLOBAL's second part, K4 turned twice within 3 s): every setting and all 16 patterns (LOAF BREAKER: 64) back to the defaults, the first break, saved at once; Bluetooth pairings kept; then a full-screen LOBOTOMIZED picture (a brain, an ice pick, the frontal lobe gone grey, a flatline: drawn from shapes, no image in flash) for 3 s or until a key or button | P1 | *Done; scenario reset.txt* |

From the brief and not built: a per-kick master chain of its own (the kick goes through CHOPPAH's chain instead,
which now has MB COMP and CLIP, A-49), separate envelope pages per layer, a preset browser with categories and
user presets (the kick is saved with the settings), and the overload guard (E-7, still open).

## 12. Future (post v1)

- Bass layer: sub / reese with 16-step note lane, sidechained to the kick slices.
- Time-stretch mode (E-5) and slice-level timestretch.
- Two-deck mode: two breaks crossfaded on K4, DJ-style.
- Ableton Link (not possible without Wi-Fi: out unless radio policy changes).
- Kit mode: one-shots on the black keys (stabs, hoovers) alongside the break.

---

## 13. Milestones

| Milestone | Scope | Exit criteria |
| --- | --- | --- |
| **M0 Pipeline** | fm1-base bootstrapped, app builds, flashes, restores to stock | App boots on the FM-1 showing CHOPPAH; stock restore tested once |
| **M1 Break player** | Bank format, ADPCM decode to pool, 1 built-in break, 16 slices on white keys, play/stop, tempo, repitch, fades | A-1, A-3, A-4, A-6, A-37 pass on device. *Built and simulator-tested 30 Oct 2026 (image 352 KB, pool 210 KB): engine, all pads, 4 FX + DJ filter + limiter, plus an 8-break Amen bank built into the image (Cyberworm's royalty-free cw_amen loops from archive.org, 22.05 kHz ADPCM, 16 slices each) chosen on ALGORITHM. Flashed to the FM-1 30 Oct 2026 (FM-1_8600); on-device acceptance checks still to run.* |
| **M2 Sequencer + generator** | Steps, swing, generator, randomize, tap tempo, patterns, persistence | A-7, A-10, A-11, A-38, B-14, B-20, B-21 |
| **M3 Remix FX + looper** | Freeze, reverse gate, retrigger, RMX, step random, pitch ±, looper with overdub | §4.3, §4.4 P0 rows |
| **M4 DSP** | Ring mod, stereo HPF, flanger, degrader, master DJ filter + limiter; overload guard | §4.5 P0 rows, N-1, N-2 |
| **M5 Breaks** | prep tool (built); loading your own breaks dropped 3 Nov 2026 | §4.7 |
| **M6 Performance mode** | Black-key pads, hold-to-loop, scrub, tape stop, p-locks, chains | §5 P0 rows |
| **M7 MIDI + sync** | Clock in/out, note/CC maps, USB audio recording flow | §4.8 |
| **M8 Release 1.0** | Manual, help cards, emulator, web installer, soak test | All P0 + N-1..N-6. *LOAF BREAKER (11 Nov 2026): the manual, the help cards, the emulator, the installer and a 30-minute simulator soak at every release are done; to go: the FM-1 tests (Firmware Hub point 5), the repository public, the breaks' rights; then `LOAF_MAJOR` 1 makes the release 1.00 (package 2.0.0, above 0.220's 1.0.21)* |

---

## 14. References

- Amen Break Generator (Revived), App Store listing: feature reference for §4
- fm1-base: https://github.com/charlesvestal/fm1-base (platform, MPL-2.0)
- felucca-bsp: https://github.com/hugelton/felucca-bsp
- X0X for the FM-1 (break part based on BB Gen): https://github.com/charlesvestal/fm1-x0x
- M-VAVE FM1 Open (official SDK, V14): https://github.com/m-vave/fm1_open
- FM-1 reverse engineering: https://github.com/AL-255/FM-1-RE
- Awesome FM-1: https://github.com/cicloid/awesome-fm-1
