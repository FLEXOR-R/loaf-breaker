# CHOPPAH: notes for Claude sessions

Firmware for the M-VAVE FM-1 (JieLi AC791N): an Amen break chopper. Spec: `docs/PRD.md` (feature IDs A-*, B-*,
E-*, N-*; milestones M0–M8). Keep the PRD in step when scope changes.

## LOAF BREAKER (FLEXOR's fork)

- The repository is FLEXOR-R/loaf-breaker (private); every push to the `loaf` branch is a release `loaf-vX.YY`
  (`.github/workflows/loaf.yml`) and updates the site, https://flexor-r.github.io/loaf-breaker/ (the firmware on it
  locked behind the Actions secret `LOAF_INSTALL_CODE`, tools/site_lock.py). Never write the code itself anywhere.
- The changelog, in French, is FLEXOR's Claude Doc (project 6f32699a-06f8-4eaa-a41a-6f03a8b20dc9, node 1fc2737b-3a82):
  after each release add a row (version, date, what changed) above the top one and put the version in the intro's
  "LOAF BREAKER 0.1xx". Keep its first part current whenever it is relevant: the summary paragraph, "Les grandes
  étapes", and "Les modifs importantes" (where things are, what was removed or replaced, shortcuts, "À savoir").
- Identity (9 Nov 2026): its own, `FM-1_2623` (fm1.toml `id_prefix`; CHOPPAH's is FM-1_86), files `loaf-breaker*`
  (`name`: loaf-breaker.fwsc, app/build/host/loaf-breaker_host), `fmhub.json` LOAF BREAKER's (author FLEXOR). CHOPPAH
  stays credited (README, site credits, the card's text, fm1.toml). The hub checklist: docs/hub/SUBMIT.md.
- Menus (9 Nov 2026, 0.177): ENV is the song (TRACKS, PLAY, DROP, BUILD); RESET is GLO's sixth part (`RST_ON`, ENV's
  reset code in track.c); the knobs' functions are EDIT KICK › KNOBS only. SYSTEM › SETUP K3 MODE (`set.novice`): BEGINNER keeps
  the essentials (0.223: the KICK one page, `kick_easy`; the FX page's PRESETS a SCENE, `fx_scene_turn`; 0.224: SEQ's BEAT, SONG's
  `trk_knob_ok`, `glo_knob_ok`, the held keys and pads (ARP's `OVL_EASY`); `KP_ADV` no longer reached in BEGINNER; 0.227: SEQ K4 LENGTH the kick's alone, 16/32/64, a drum lane 16 ("EXPERT MODE"),
  BEGINNER turned on sets the kicks 32 (`novice_lengths`); the FX scenes then MAGIC EQ, MAGIC MBC on the slot shown); the host simulator starts EXPERT (scenarios, the manual's screens), the
  FM-1 and the web emulator BEGINNER. The manual opens with "Start in five minutes" (the hub card's `cheat` link).
- Screen and knobs (0.178): the screen's rows are remembered as hashes (`rows_send` in common.c): a full-width strip
  sends only its changed rows. Anything drawing on the screen other than through `cv_blit` must forget its rows
  (`rows_forget`; `lcd_fill` is wrapped). `changed()` applies once a pass (`apply_flush`); a kick-only or FX-only change
  uses `changed_kick()` / `changed_fx()`. SYSTEM › MIDI K3 METER (EXPERT only, 0.213) shows CPU / IMG / FR on the device. CI runs a 30-minute
  random soak (`tools/soak.py`, seed = run number) at every release.
- Names (10 Nov 2026, 0.191): what the code calls TERROR, TERROR 2, TERROR + the screen and the docs call DISTO,
  SCREAM, DISTO + (D+ TYPE, D+ SCREECH ..); the black key DST. Keep newcomers' names on the screen; TERROR stays the
  genre (README's tagline) and a STYLE.
- Breaks (10 Nov 2026, 0.193): FLEXOR chose the bank on a listening page of all 70 breaks as the FM-1 holds them
  (https://claude.ai/artifact/MCakoDjuGTXjMLFiobM1k3; the choice in its store, `tri/choix`): 9 breaks, NUSKOOL first (FLEXOR's order; LO-FI out in 0.200 for room).
- Patterns (10 Nov 2026, 0.197): 64, four groups of 16 (`NGRP`, `NPAT_ALL`); the group in RAM is pats.p/kn/hits and
  steps.b (`set.pgrp`; `set.pattern` its slot; `pat_cur()` 0..63; `pat_go()` brings one in). Each group has its own
  objects (OBJ_PG0/KG0/SG0 + g; NG0 unused since 0.209); SAVE (`store_all`) writes 7 (with OBJ_KLAB). Leaving a group with unsaved changes while playing is
  refused ("SAVE OR STOP", FLEXOR's choice: never a cut in the sound). Track items, `drop_after`, `ui.drop_pat` and
  `pat_next` are 0..63 (or 1..64); `drop_to` stays a slot (0xFF: another group's, read in at the drop).
- MELODY and KEYS (0.198-0.208) removed in 0.209 (FLEXOR: "kicks and drums only"): perc.c's PV_STAB is an empty place
  (perc_t's layout in the saves), DR_STAB gone (NDRUMS 4; `NDRUMS_SAVED` 5 for pats.hits), OBJ_NG0 + g no longer
  saved or read (SAVE writes 7 objects), settings' pstab/smem/sslot/mel_vol/mel_ver unused, st[k][2] bits 3-7 unused.
  Don't bring a melody back without FLEXOR asking.
- Domains (10 Nov 2026, 0.201; 0.200 is the layout before, FLEXOR may want it back): the header and the tabs name the
  domain (`DOM[]`, `dom_name()` in main.c): LIVE, SOUNDS, PATTERN, SONG, MIX, EFFECTS, SYSTEM. SOUNDS' sources: KICK,
  DRUMS (einst 1-3), BREAK (PG_GLOBAL `GP_BREAK`, GLO's old PITCH code, K4 LEVEL); SEL + EDIT the
  next; EDIT again: the KICK's sections only (0.227, FLEXOR: the 0.226 round to the drums taken back), a drum's the next drum. PATTERN's GROOVE is PG_GLOBAL `GP_FILL` (SWING, LATCH, DRUM, LENGTH), reached from SEQ's ALGORITHM and SEL + ARP;
  LEDs light EDIT / SEQ for those two. SYSTEM's parts `GLO_ORD` (MIDI, SETUP, EFFECTS, FX SET, RESET); RESET ALL is
  RESET's last item, EVERYTHING (15 turns). MIX's second part is SENDS (FX, DRUMS).
- Knob presets (10 Nov 2026, 0.203): STYLE's PRESETS goes on past the eight styles to PRESET 1-4 (track.c, `KPRESETS`):
  `steps.kpre[4][9]` (appended to steps_t: four codes, four values 0..200, stored), the same for every kick; an empty
  one gives back `kcust` (the knobs before PRESETS turned); SAVE on STYLE with one picked stores them (`kpre_store`).
- KICK LAB (10 Nov 2026, 0.204): the KICK menu's sixth section LAB (TAIL, TAIL +, STACK, TOOLS; kick.c klab_*),
  `kick_lab_t` (kick.h) per kick in its own object `klab` (OBJ_KLAB 0xB6000: cur + mem[10], swapped with the kick in
  kick_mem_go, copied with COPY K n, off with a built-in PRESET / RESET); engine `kick_set_lab`, `kick_set_stack`
  (a second kick voice, kv2, until STACK's hand-over), `kick_set_solo`. All 0 renders bit for bit as 0.203: keep it so.
  The tabs drop the "KICK" label on the KICK menu (six sections).
- KICK menu by anatomy (10 Nov 2026, 0.205, FLEXOR's choice): KSECT ATTACK, BODY, TAIL, DIRT, TONE, KNOBS (kick.c);
  app_ui_init opens it on BODY (ui.ksect 1, its MIX: the first EDIT still kpage 0); the parts in a row at the
  picture's foot (kick_chip; 0.207: a full-width bar of tabs, FONT_T if it fits, the wave up: its middle y 45, 29
  px each way). Views (ui.kview, EDIT held): RGB (kick_rgb, 500 ms), WAVE, then CLASSIC (ui.kclassic).
- GLO EFFECTS K3 WAVE (`set.wstyle`: REKORDBOX / SERATO, common.c serato_col; the kick's RGB and cdj.c) and K4 KICK
  VIEW (`set.kfocus`: the whole kick / the part edited: kick.c kv_ms, kv_kick) (10 Nov 2026, 0.206).
- TUTORIAL (0.211, FLEXOR: "a sober name"): ui/tutor.c (included before screen.c), TUT[] steps (lesson, condition TC_*,
  keys and buttons to blink, text), `set.tut` (appended: 0 off, 1..TUT_N the step, 0xFE done; a new FM-1 starts it,
  an older save not), the counters tut_* in common.c; the header draws the step (screen.c); SETUP K3 left of
  BEGINNER; GLO quits, SEL + GLO skips. 0.212 (FLEXOR): HOME first (on PERFORM already, HOME stays: tut_home_stay),
  OUT's K4 the built-in kicks (0.223: BEGINNER's KICK page, K1; 18 steps). 0.213: a step done waits 5 s (`TUT_WAIT`, the header counting) only where there is
  something to hear (TUT[].wait: slices, effects, kicks, DISTO, steps, the drop); a button step goes on at once. Keep
  its steps true when a control they teach moves.
- EDIT ZOOM (0.216, FLEXOR: "a wider view"): `set.ezoom` (appended after `tut`; 0 AUTO), SYSTEM › EFFECTS PRESETS,
  EZ_MS[] in common.c (250 ms .. 2 s): kick.c kv_ms() and perc.c draw_perc_big's pw_ms; the time drawn under the HZ.
  The picture's cost grows with its time (kick_rgb ticks the kick: 2 s = 88,200 samples a redraw, main loop only).
- SETUP (0.213, FLEXOR): the knobs in the screen's order, K1 SKIN, K2 WAVE ZOOM (PRESETS too), K3 MODE, K4 AUTO SAVE;
  the METER moved to MIDI's K3, EXPERT only (BEGINNER turns it off).
- Drum presets (11 Nov 2026, in progress): FLEXOR picks styled presets by ear on listening pages (rendered on the
  host); his picks, the prototype engine and the plan in `docs/sounds/PERC_PRESETS.md`. His aim: BEGINNER's drum pages
  only a PRESET and an FX, EXPERT every setting (accessibility: surf the machine first, the depth after).
  0.225: the engine's extras (perc.h PX_*: DROP, DTIME, DIRT, LIFT, ROOM; `perc_set_x`), per memory in `smem` (the
  MELODY's old place, `perc_x()`, `set.pxv` = PX_OK), FLEXOR's presets PP_SN / PP_CP in ui/perc.c with their FX,
  pages SOUND / SHAPE (K4 DIRT) / MORE, BEGINNER's PRESET / FX / LEVEL / TUNE.
- RESET (0.209): after a reset its page's knobs do nothing for 3 s (`RST_COOL`, `ui.rst_done`, rst_cool()), DONE shown.
- No Bluetooth, ever (FLEXOR's rule). The GLOUPI BREAKS start-up photo must never come back into the repository.
- Fonts: `font_trim` is on (fm1.toml, 9 Nov 2026). LOAF BREAKER draws in FONT_T, FONT_XS and FONT_B only (cv_text2 for
  bigger: those doubled). fm1-base still generates S, M (~5 KB trimmed) and L (~9 KB trimmed, 31 px), left out of the
  image as never read. FLEXOR asked to be reminded of them whenever bigger or other text would help (a big tempo, a
  title): say what one would cost then.

## Layout

Porting to other hardware (FLEXOR's question, 10 Nov 2026): `docs/PORTING.md` (the 31 `plat_*` the app calls, 18
without Bluetooth; the screen, the controls, the order of the work). Keep it current when the app calls a new one.


`app/app.c` is the core (settings, patterns, MIDI, the render); it ends by including `app/ui/*.c` in order (common,
fx, pads, keys, bta, global, ovl, seq, kick, perc, mixer, track, tutor, screen, demo, cdj, main): one unity build, so a page file sees what the
files before it define. `app/dsp/*.c` are separate `o2` units (brk, brk_fx, kick, perc, ovl, …).

## Hard rules

- `app/dsp/` stays platform-free: no `plat_*`, no fm1-base headers, no libc beyond `<stdint.h>`. It must build with
  plain `cc` (tests/engine) and as fm1-base `o2` units.
- **No double precision** anywhere in `app/` (the device build rejects it): `f` suffix on every float literal,
  no `math.h` in dsp (use `brk_math.h`). `tests/engine/run.sh` builds with `-Wdouble-promotion -Werror`.
- No 64-bit division on the device (no libgcc `__udivdi3`): keep sample maths in 32 bits.
- The render (`app_render`, `brk_render`) runs in the audio interrupt: no flash writes, no blocking, no
  allocation. Main loop → render through `brk_*` setters (single words) and the key queue.
- Pool RAM budget (PRD 8.2): canvas 240×60 (28 KB) + PCM buffers; the build wants 8 KB of the 344 KB spare.
- Flash writes stall audio: only on SAVE or while stopped.
- Settings (`settings_t`, magic CHP7) only grow at the end: append new fields after the last one (`fxx`, each FX slot's extras) and
  give them defaults in `settings_defaults()`; a shorter (older) save loads into the front (`SET_SIZE_MIN`: the
  offset of the first appended field, `rsv2`, the dropped fills' bytes; keep it there).
  Anything else needs a new magic (saves lost). Patterns are their own object (`OBJ_PAT`, 0xDA000, magic PAT1).
- No Bluetooth in LOAF BREAKER (FLEXOR's rule): app/fm1.toml has `ble = false` and `bt_audio = false` (CHOPPAH's
  were true: ~40 KB and ~37 KB of flash). Their app hooks (`APP_BLE_AT_BOOT`, `BP_MQ_ATTR` in app.h) stay as
  CHOPPAH's, inert; never turn either back on.
- Flash: the app slot is 581,564 B (`APP_SLOT` 0x8DFBC in base/tools/fm1pkg_make.py: not 581 KiB); 577,288 B used at
  0.192 with a 357 KB bank; 561,424 B at 0.220 (9 breaks); 568,492 B at 0.225 (the drums' extras): **~13 KB left**.
  RAM .data+.bss 93,292 of 98,304 B at 0.225 (the drums' room, 5.8 KB of 16-bit lines, is in RAM): ~5 KB left; the
  pool has ~10.8 KB (it could take the room's lines). The break bank (`CH_BANK`) is most of it.
  New features are welcome before 1.0 (FLEXOR, 11 Nov 2026: "not that close to 1.0, I want to perfect the firmware");
  anything big needs something taken out first (ask FLEXOR what).
  Block-rate engine code is `BRK_COLD` (built for size) and calls the shared maths (`brk_exp2f_c` and the like,
  brk_math.c; the UI too) instead of an inline copy; the FX chain's per-sample loop (`fx_samples`) is `BRK_RAM`:
  .ram_text on the FM-1 (fast, packed in flash), so nothing it reaches may be a call (`BRK_INL` in brk_fx.c forces
  inlining; the build fails on "ram_text contains calls"; flash code reaches it through a pointer). CPU: the render
  must stay under its 5.8 ms block, or audio glitches and the watchdog resets the FM-1; `APP_OVERLOAD` sheds FX slots
  as a last resort (PRD E-7). Compare an engine change's cost on the host against the last release before shipping (`APP_SYS_TEXT` in app.h draws the platform's screens in CHOPPAH's faces,
  saving fm1-base's fonts): check `./build.sh`'s image line;
  the canvas is 240×60 (`APP_CANVAS_PX`), so draw in strips of ≤ 60 rows (draw_fx / draw_key draw two bands).
- Breaks: `breaks/breaks.txt` (paths relative to `breaks/`) marks with `*` the WAVs built into the firmware;
  `./build.sh` regenerates `app/bank_data.h` (never commit it, never edit it). The bank has 400 KB (`IMAGE_CAPACITY`
  in `tools/prep_breaks.py`) and 16 slots, a break at most `CH_PCM_MAX` 106,496 samples (pool ~326 of 344 KB). The breaks are the bank's only: brk_synth is the no-bank fallback (and
  the tests'). No loading of samples of your own (dropped from scope), and the breaks stay 2 bars (32 steps; a one-bar break plays twice, LO-FI 165 was one in 0.196-0.199).
- Releases are automatic: every push to `loaf` is the last `loaf-vX.YY` + 1 (`scripts/loaf_version.sh next`; past
  0.99 on to 0.100 ...). `./LOAF_MAJOR` is the floor of X: set it to 1 and push for 1.00 (then 1.01 ...). The
  package's own number (fm1-base's X.Y.Z, two digits each) is `scripts/loaf_version.sh pkg`: 0.220 is 1.0.21, 1.00
  is 2.0.0, always above the one before (`check` in CI). Don't tag by hand. (`scripts/next_version.sh` and
  `VERSION` are CHOPPAH's main branch's.)
- **Always push to GitHub after changes** (Laurens' standing rule, 4 Nov 2026): commit and push as soon as a change is
  done, before moving on. Finished, tested work goes to main (a release); work in progress that doesn't build or fit
  yet goes to a branch of its own and is pushed there.

## The update screen

The screen the FM-1 shows while an update is written ("UPDATING", "DO NOT UNPLUG", the bar) is the update loader's,
built from felucca-bsp (`base/bsp/loader/`), which has no hook: `build.sh` copies `app/loader/ldr_screen.c` over
`base/bsp/loader/ldr_screen.c` for a device build and puts the original back when it ends (never commit the
submodule changed). Keep its function names, the panel's start and RAM-only buffers as felucca-bsp's: the loader runs
while the app area is rewritten. It ships in the package, so it shows when that package is installed.

## Site

`base/tools/make_site.py` builds the GitHub Pages site; `tools/site_style.py` then gives it CHOPPAH's look
(`app/site/site.css`; the installer and the emulator, fm1-base's pages, also `app/site/install.css` and
`app/site/emu.css`: only their style, never their markup, ids or script) and puts the interactive control map (`app/site/controlmap.html`) into the manual's section
"The control map". When a page's controls change, update that file's `PAGES` list with the manual. Manual screenshots
come from the host scenarios (`app/build/scenarios/*.png`, copied into `app/docs/img`).

## Commands

```
./build.sh test        # engine tests (fast, run after every dsp change)
./build.sh             # firmware (needs scripts/bootstrap.sh, Docker/OrbStack on macOS)
./build.sh --host      # host simulator
./build.sh flash       # macOS Terminal only: the sandboxed shell has no USB access
```

## Platform facts (fm1-base 2d8ed74, Nov 2026)

44.1 kHz stereo, 256-frame blocks; 581,564 B app flash; 96 KB RAM + 336 KB pool; 240×240 screen; 27 keys
(16 white / 11 black, `PLAT_WHITE_KEYS` / `PLAT_BLACK_KEYS`); 14 buttons (`B_*`), 7 encoders (`EN_*`), MASTER pot.
Store objects ≤ 3840 B; data area 0x97000–0xDFFFF, XDATA0 0x93000–0x96FFF, XDATA1 0xE5000–0xFBFFF (keep bytes
0xF00–0xF07 of each XDATA sector erased).
