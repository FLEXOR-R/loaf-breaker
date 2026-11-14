# CHOPPAH: notes for Claude sessions

Firmware for the M-VAVE FM-1 (JieLi AC791N): an Amen break chopper. Spec: `docs/PRD.md` (feature IDs A-*, B-*,
E-*, N-*; milestones M0–M8). Keep the PRD in step when scope changes.

## LOAF BREAKER (FLEXOR's fork)

- The repository is FLEXOR-R/loaf-breaker (public from 11 Nov 2026, FLEXOR). **Work goes to `dev`** (12 Nov 2026, FLEXOR:
  not twenty public versions a day): each push there replaces the one **draft** release "LOAF BREAKER TEST" (loaf-breaker-A.B.C-testN.fwsc; a draft,
  seen by FLEXOR only: the hub looks at the newest release, pre-releases too, and a TEST pre-release made its check fail;
  the splash "A.B.C TEST N"; no site, no soak, no tag). **A release only when FLEXOR asks** ("release", "publie"): dev
  pushed to `loaf` (`git push origin dev:loaf`, a fast-forward), whose every push is a release `loaf-vA.B.C` (0.9.1 on; 0.02-0.267 were X.YY)
  (`.github/workflows/loaf.yml`; CLAUDE.md alone releases nothing) and updates the site, https://flexor-r.github.io/loaf-breaker/ (0.261: the firmware on it
  open, no install code; tools/site_lock.py and the secret LOAF_INSTALL_CODE no longer used). Never write that old code anywhere.
- The changelog, in French, is FLEXOR's Claude Doc (project 6f32699a-06f8-4eaa-a41a-6f03a8b20dc9, node 1fc2737b-3a82):
  after each release (not a TEST) add a row (everything since the last one) (version, date, what changed) above the top one and put the version in the intro's
  "LOAF BREAKER 0.9.x". Keep its first part current whenever it is relevant: the summary paragraph, "Les grandes
  étapes", and "Les modifs importantes" (where things are, what was removed or replaced, shortcuts, "À savoir").
- Identity (9 Nov 2026): its own, `FM-1_2623` (fm1.toml `id_prefix`; CHOPPAH's is FM-1_86), files `loaf-breaker*`
  (`name`: loaf-breaker.fwsc, app/build/host/loaf-breaker_host), `fmhub.json` LOAF BREAKER's (author FLEXOR). CHOPPAH
  stays credited (README, site credits, the card's text, fm1.toml). The hub checklist: docs/hub/SUBMIT.md.
- Menus (9 Nov 2026, 0.177): ENV is the song (TRACKS, PLAY, DROP, BUILD); RESET is GLO's sixth part (`RST_ON`, ENV's
  reset code in track.c); the knobs' functions are EDIT KICK › KNOBS only. SYSTEM › SETUP K3 MODE (`set.novice`): BEGINNER keeps
  the essentials, as FLEXOR chose them on a checklist (0.229; https://claude.ai/artifact/75VV4U2fH5iouT8789feZN, its
  store `choix/beginner`): the KICK three pages on OUT's part (`kick_easy`, `KEASY[]`, `keasy_pg`: KICK, SHAPE, MIX; the foot's tabs, the
  header's KICK SN HH CP with SELECT round them, 0.230), the
  drums PRESET / FX / LEVEL / DECAY, SEQ as EXPERT's but the break's two halves always on top (0.233), no step menu, no GROOVE in its lanes' round (0.231) and K4 LENGTH the kick's alone (16/32/64, a drum
  lane 16; BEGINNER turned on sets every drum 32 (0.9.32, FLEXOR; the kicks 16 in 0.246-0.9.31), `novice_lengths`; a new pattern's every drum 32 too, whatever the mode, `PAT_ALL32`), the FX chain two slots (`fx_nslots`, the others
  muted in `fx_apply`) and a short list (`FX_EASY`) then MAGIC EQ, MAGIC MBC; SONG whole since 0.9.22 (FLEXOR; `trk_knob_ok` always 1), `glo_knob_ok`, the
  held white keys (no knobs), the black keys' knobs as EXPERT's (0.232); blank knobs say EXPERT MODE (`easy_no`; the DST and SUB keys' restriction gone in 0.244); `KP_ADV` no
  longer reached. Gone in 0.229: the FX scenes, SEQ's BEAT presets, MB KICK and MB OTT. A new FM-1 asks its mode
  (tutor.c `TUT_ASK`, set.tut 0xFD; the simulator with LOAF_ASK=1), and so does RESET ALL, as a first start (0.236:
  after its screen, `ui` cleared and `app_ui_init`: the start screen, then the question). The host simulator starts EXPERT (scenarios, the
  manual's screens), the FM-1 and the web emulator BEGINNER. The manual opens with "Start in five minutes" (the hub card's `cheat` link).
- Screen and knobs (0.178): the screen's rows are remembered as hashes (`rows_send` in common.c): a full-width strip
  sends only its changed rows. Anything drawing on the screen other than through `cv_blit` must forget its rows
  (`rows_forget`; `lcd_fill` is wrapped). `changed()` applies once a pass (`apply_flush`); a kick-only or FX-only change
  uses `changed_kick()` / `changed_fx()`. SYSTEM › MIDI K3 METER (EXPERT only, 0.213) shows CPU / IMG / FR on the device. CI runs a 30-minute
  random soak (`tools/soak.py`, seed = run number) at every release.
- Names (10 Nov 2026, 0.191): what the code calls TERROR, TERROR 2, TERROR + the screen and the docs call DISTO,
  SCREAM, DISTO + (D+ TYPE: FOLD CLIP CRUSH RECT TUBE, 0.255 HALF SINE WRAP FUZZ, `KICK_T_TYPES`; D+ SCREECH ..); the black key DST. Keep newcomers' names on the screen; TERROR stays the
  genre (README's tagline: HARDCORE - GABBER - BREAKCORE - TERROR - SPEEDCORE, FLEXOR 11 Nov 2026) and a STYLE.
- Breaks (10 Nov 2026, 0.193): FLEXOR chose the bank on a listening page of all 70 breaks as the FM-1 holds them
  (https://claude.ai/artifact/MCakoDjuGTXjMLFiobM1k3; the choice in its store, `tri/choix`): 9 breaks, NUSKOOL first (FLEXOR's order; LO-FI out in 0.200 for room; AMEN15 174 out in 0.9.30, FLEXOR, for the drums' 64 steps: 8 breaks, 298 KB). 0.9.34 (14 Nov 2026, FLEXOR's Break Swap on https://claude.ai/artifact/JqvNy6DuTPBk6WBZVDZHww, store `choix/breaks`): his three of `breaks/flexor/` (sampled from artists' records, not royalty free) in, NUSKOOL and JUNGLE out: 9 breaks, AKIRA 190 first, 314.5 KB, his three sliced on an even grid (FLEXOR: EVEN GRID; breaks.txt `*=`, `-=`); image 573,840 B, ~7.7 KB of flash left. FLEXOR wants more options on that page (to come).
- Patterns (10 Nov 2026, 0.197): 64, four groups of 16 (`NGRP`, `NPAT_ALL`); the group in RAM is pats.p/kn/hits and
  steps.b (`set.pgrp`; `set.pattern` its slot; `pat_cur()` 0..63; `pat_go()` brings one in). Each group has its own
  objects (OBJ_PG0/KG0/SG0 + g; NG0's first is KICK 11-20 since 0.9.2, the others unused); SAVE (`store_all`) writes 10 (with OBJ_KLAB, OBJ_KMEM2, OBJ_KFXM, OBJ_DX). Every drum 16, 32 or 64 steps (0.9.30, FLEXOR): the snare's, hat's, clap's bars 3-4 of all four groups in `dx` (OBJ_DX = OBJ_NG0 + 2, read through `dx_of`; `dx_tp` for a drop's pattern from another group), 64 = PAT_LEN bits 5-7 (`LEN64`), SEQ lanes S3 S4 H3 H4 C3 C4; their steps 33-64 share 1-32's stepb. Leaving a group with unsaved changes while playing is
  refused ("SAVE OR STOP", FLEXOR's choice: never a cut in the sound). Track items, `drop_after`, `ui.drop_pat` and
  `pat_next` are 0..63 (or 1..64); `drop_to` stays a slot (0xFF: another group's, read in at the drop).
- MELODY and KEYS (0.198-0.208) removed in 0.209 (FLEXOR: "kicks and drums only"): perc.c's PV_STAB is an empty place
  (perc_t's layout in the saves), DR_STAB gone (NDRUMS 4; `NDRUMS_SAVED` 5 for pats.hits), OBJ_NG0 + g no longer
  saved or read (SAVE wrote 7 objects; 8 since 0.9.2, OBJ_NG0 holding KICK 11-20), settings' pstab/smem/sslot/mel_vol/mel_ver unused, st[k][2] bits 3-7 unused.
  Don't bring a melody back without FLEXOR asking.
- Domains (10 Nov 2026, 0.201; 0.200 is the layout before, FLEXOR may want it back): the header and the tabs name the
  domain (`DOM[]`, `dom_name()` in main.c): LIVE, SOUNDS, PATTERN, SONG, MIX, EFFECTS, SYSTEM. SOUNDS' sources: KICK,
  DRUMS (einst 1-3), BREAK (PG_GLOBAL `GP_BREAK`, GLO's old PITCH code, K4 LEVEL); SEL + EDIT the
  next; EDIT again: the KICK's sections only (0.227, FLEXOR: the 0.226 round to the drums taken back), a drum's the next drum. PATTERN's GROOVE is PG_GLOBAL `GP_FILL` (SWING alone since 0.244: FILL gone), reached from SEQ's ALGORITHM and SEL + ARP;
  ARP itself is the OVERLAY page again (0.244, FLEXOR: ovl.c, PG_OVL; 0.9.27: its effect touched last is the ARP key's, `ovl_last` = set.ovl_pad & 15, and its keys of the effects on lit);
  LEDs light EDIT / SEQ for those two. EXPERT's SEQ (0.234): BREAK above the first lane (`ui.seq_brk`, `SEQ_BRK`), K4
  the break's rows shown (`brk_view()`, settings' `brkv`: the old mel_ver byte, tagged 0xB0, since settings_t is at
  3840 bytes: no field can be appended any more without a new magic or freeing one). SYSTEM's parts `GLO_ORD` (MIDI, SETUP, EFFECTS, FX SET, RESET); RESET ALL is
  RESET's last item, EVERYTHING. RESET (0.239, FLEXOR): K4 picks, K2 five clicks, then GLO within 5 s (`ui.rst_ask`,
  `rst_confirm`); ALGORITHM (and SELECT, 0.9.2) there goes to SYSTEM's parts. MIX's second part is SENDS (FX, DRUMS, ARP K3: the ARP key's THRU, ovl_pad bits 6-7, BREAK KICK THRU, `arp_thru_turn`, 0.9.29; LEVELS' K4 too until 0.9.30. Its BREAK / KICK route the kick around the chain: the limiter then after it joins, `brk_fx_late_limit` + `brk_fx_add_dry`, 0.9.31, FLEXOR: it sounded fatter, unlimited).
- Knob presets (10 Nov 2026, 0.203): STYLE's PRESETS goes on past the eight styles to PRESET 1-4 (track.c, `KPRESETS`):
  `steps.kpre[4][9]` (appended to steps_t: four codes, four values 0..200, stored), the same for every kick; an empty
  one gives back `kcust` (the knobs before PRESETS turned); SAVE on STYLE with one picked stores them (`kpre_store`).
- KICK LAB (10 Nov 2026, 0.204): the KICK menu's sixth section LAB (TAIL, TAIL +, STACK, TOOLS; kick.c klab_*),
  `kick_lab_t` (kick.h) per kick in its own object `klab` (OBJ_KLAB 0xB6000: cur + mem[10], swapped with the kick in
  kick_mem_go, copied with COPY K n, off with a built-in PRESET / RESET); engine `kick_set_lab`, `kick_set_stack`
  (a second kick voice, kv2, until STACK's hand-over), `kick_set_solo`. All 0 renders bit for bit as 0.203: keep it so.
  The tabs drop the "KICK" label on the KICK menu (six sections).
- KICK menu by anatomy (10 Nov 2026, 0.205, FLEXOR's choice): KSECT ATTACK, BODY, TAIL, DIRT, FX1, FX2, MORE (kick.c;
  0.9.29, FLEXOR: FX1 and FX2 sections of one part each, ALGORITHM there the effect's parts (`fx_part_turn`, the FX page's chips), no ksect_row on them; MORE (KNOBS until 0.9.28) 21 27 22-25 31 11 12: KNOBS, ASSIGN, K1-K4 SET, TOOLS, EQ B, EQ S, nine parts (KSECT rows of 9; `ksect_row` narrows its margins to fit); KNOBS' STYLE named ASSIGN in 0.9.2);
  app_ui_init opens it on ATTACK's PRESET (0.253, FLEXOR: KP_PRESET, part KP_PART_PRESET, K1 the built-in kicks alone; ui.ksect 0; 0.205-0.252 on BODY's MIX); the parts in a row at the
  picture's foot (0.264, FLEXOR: SELECT the tabs at the top, its sections, kick_sect_turn; ALGORITHM those at the
  bottom, its parts; EXPERT's drums: SELECT round SN HH CP, the KICK by SEL + EDIT; 0.266: EXPERT's EDIT goes round the sounds, KICK SN HH CP) (kick_chip; 0.207: a full-width bar of tabs, FONT_T if it fits, the wave up: its middle y 45, 29
  px each way). Views (ui.kview, EDIT held): RGB (kick_rgb, 500 ms), WAVE, then CLASSIC (ui.kclassic); 0.231 the drums'
  big view too (perc_rgb into kv_rgb, shared: kv_sig / pw_sig say whose), one state for all.
- HOME (0.9.2, FLEXOR: the main page): PERFORM, GENERATOR, SETTINGS (PG_HSET, global.c hset_*: K1 VIEW `ui.blocks` kept in
  `bkeys.bk[BK_VIEW]`, K2 WAVE TYPE `wstyle`, K3 WAVE ZOOM `wzoom` (SETUP's K2 before), K4 BREAK TIME (MIDI's K4 before)).
  Navigation (0.9.2): on pages with tabs SELECT the top ones, ALGORITHM those below (EFFECTS: the slot / MAIN MORE MOD;
  SONG, MIX, SYSTEM, OVERLAY: their tabs; PATTERN: SELECT the lanes as ALGORITHM, 0.9.3); where those two move everywhere,
  the page's button again goes HOME (FX, ENV, LFO, GLO, ARP, SEQ (held: the clear, as before); not EDIT, not the kick's FX; GLO still confirms RESET and quits the TUTORIAL). The drums go AROUND the
  FX by default (`drums_chain` 0 in the defaults; a saved THRU stays). On HOME, SEL + PRESETS the kick (0.9.26, FLEXOR;
  elsewhere the pattern).
- **A saved setting never changes at a restart** (FLEXOR, 12 Nov 2026): defaults are for a new FM-1 and resets only;
  never force a value at power-on once it has been saved.
- KICK 1-20 (0.9.2, FLEXOR): `KICK_MEMS` 20, `KMEM_A` 10 (the first ten's arrays as saved before); 11-20 in `km2`
  (OBJ_KMEM2 = OBJ_NG0, pool RAM); always reach a memory through `kmem_at`, `ksmem_at`, `kx_at`, `kxh_at`, `klmem_at`.
  Each kick's PRESET name: `kick_lab_t.pre` (1 + OUT K4's place; 0 the memory's own built-in, `kpre_mem`).
  Each kick's FX section (EQ B/S, FX1, FX2: set.keq, kfx, kfxx, kmbc) is its own since 0.9.26 (FLEXOR): `kfxm`, 16-bit
  packed, OBJ_KFXM = OBJ_NG0 + 1 (0xBA000), `kfx_put` / `kfx_get` in kick_mem_go and COPY; SAVE writes 9 objects. In the
  KICK menu PRESETS is the kick; SEL + PRESETS K1-K4 SET's function, ASSIGN's style; in its FX section (0.9.29, FLEXOR) PRESETS the effect (FX1, FX2; nothing on EQ B, EQ S), SEL + PRESETS the kick. SEL + ARP there (and on a drum's page) copies, then on another kick pastes (`kick_clip`, `perc_clip` in main.c; `kick_copy_from`), GROOVE elsewhere. SEL + ALGORITHM where ALGORITHM is the break: BREAK TIME (`sel_bt_page`); SEL + PRESETS the kick on HOME, PATTERN, BREAK, MIX, OVERLAY, SYSTEM but SETUP / EFFECTS; SETUP's PRESETS the skin; SEQ's ALGORITHM every lane top to bottom, round (`seq_lane_step`), SELECT the views FULL, DRUMS, SETTINGS (`seq_view_turn`; SETTINGS is GP_FILL: SWING, BREAK ROWS, FOLLOW in bk[17] `BK_SEQ`; 0.9.31, FLEXOR). TRACKS: a DROP item's K1 its build's kick, K3 its STUTTER (`b_stut`, bit 0 of its `trk_brk` byte), K4 GAP KEEPS (`b_gapm`, bits 1-2: NOTHING, BREAK, DRUMS, ECHOES); FLEXOR wants more drop settings later.
- TERROR and SPINE, the seventh and eighth built-in kicks (0.9.32, FLEXOR, chosen by ear: PRD A-54i; SPINE TERROR with DRIVE 50,
  D+ TYPE RECT, SUB 766 ms): `KICK_PRESET_TERROR`, `KICK_PRESET_SPINE`, their kick_t part in
  kick.c PRESET / kick_preset, the rest (SYNTH, TAIL + DRIVE, KICK EQ LOW) in ui/kick.c `kpre_extra`; the kick memories
  begin as the eight in order, round (0.9.33, FLEXOR: KICK 7 TERROR, KICK 8 SPINE; `kmem_extra` / `kmem_extras` their rest and
  their PRESET stored, on a new FM-1, RESET ALL, ALL KICKS; a saved memory never changed: a PRESET never stored is read as
  the first six, round, `KICK_PRESETS_MEM`). Kick listening pages: the host's
  `kset NAME VALUE` (ui/main.c) sets a kick exactly as its knobs show it.
- WAVE TYPE (`set.wstyle`: REKORDBOX / SERATO; HOME › SETTINGS K2 alone since 0.9.25, GLO EFFECTS' K3 before; SYSTEM shows
  none of HOME's SETTINGS any more, FLEXOR; common.c serato_col; the kick's RGB and cdj.c) and EDIT
  VIEW (KICK › KNOBS › TOOLS K3 since 0.9.29, SYSTEM › EFFECTS K4 before; `set.kfocus`: the whole kick / the part edited: kick.c kv_ms, kv_kick) (10 Nov 2026, 0.206).
- TUTORIAL (0.211, FLEXOR: "a sober name"): ui/tutor.c (included before screen.c), TUT[] steps (lesson, condition TC_*,
  keys and buttons to blink, text), `set.tut` (appended: 0 off, 1..TUT_N the step, 0xFE done, 0xFD the mode asked (0.229); a new FM-1 asks,
  an older save not), the counters tut_* in common.c; the header draws the step (screen.c); SETUP K3 left of
  BEGINNER; GLO quits, SEL + GLO skips. 0.212 (FLEXOR): HOME first (on PERFORM already, HOME stays: tut_home_stay),
  OUT's K4 the built-in kicks (0.223: BEGINNER's KICK page, K1). 0.235: 23 steps in eight lessons (0.244: 25, the OVERLAY page; 0.246: 24, the ARP key's step gone, FLEXOR: not relevant; 0.252: 26, THE PATTERN before THE KICK, then the kick and the snare heard stopped on keys 1-4: `tut_heard`, a key after the last knob turn; 0.9.3: 31, lesson HOME after the break's: GENERATOR, SETTINGS, KNOB 1 the slices, PERFORM, HOME held, TC_HGEN..TC_HHOLD), BEGINNER as FLEXOR
  chose it (ALGORITHM a break, ARP + K1, K2 DISTO, EDIT to SHAPE and K3 TAIL, SELECT to the SNARE and its PRESET / FX,
  the FX page's PRESETS). 0.213: a step done waits 5 s (`TUT_WAIT`, the header counting) only where there is
  something to hear (TUT[].wait: slices, effects, kicks, DISTO, steps, the drop); a button step goes on at once. Off a step's page the header says the way back (0.248, `tut_away`); the page buttons a step doesn't ask for are held back (0.249, `tut_btn_ok`); the steps say KNOB 1, not K1. AUTO SAVE (0.250, FLEXOR): off while the tutorial runs, on at its end or quit; the first start's BEGINNER turns it on. Keep
  its steps true when a control they teach moves.
- EDIT ZOOM (0.216, FLEXOR: "a wider view"): `set.ezoom` (appended after `tut`; 0 AUTO), KICK › KNOBS › TOOLS K4 "DRUM ZOOM" (0.9.29; SYSTEM › EFFECTS PRESETS before),
  EZ_MS[] in common.c (250, 500, 750 (0.257), 1000, 1500, 2000 ms; read with `ez_idx()`: EZ_TAG | index, an older save's 3.. one on): kick.c kv_ms() and perc.c draw_perc_big's pw_ms; the time drawn under the HZ.
  The picture's cost grows with its time (kick_rgb ticks the kick: 2 s = 88,200 samples a redraw, main loop only).
- Skins (0.237, FLEXOR's for 1.0): 24 in common.c (SKIN_NAME, SKIN_RGB; picked and named on the artifacts
  https://claude.ai/artifact/QpXSifxqofMdzMw1AM23yr, .../KuYXSJbwFsLJJUTTZRCUU6, .../PZo1tMVjmi2HcoLLJtY3tV), RED
  (SKIN_DEFAULT 10; CHOPPAH 0.237-0.239) to start; names in full (0.239: a foot value wider than 54 px is drawn in FONT_T, 2 px left;
  BREADCRUMBZ, 11 letters, is the widest that fits); set.skin
  carries SKIN_NEW (bit 7), an older save's 0..17 mapped by SKIN_OLD; read it with `skin_idx()`. SETUP shows two rows
  of twelve swatches.
- SETUP (0.213, FLEXOR): the knobs in the screen's order, K1 SKIN, K2 (WAVE ZOOM until 0.9.1: HOME SETTINGS), K3 MODE, K4 AUTO SAVE
  (0.9.26, FLEXOR, asked several times: never while PLAY runs, once PLAY has been stopped 20 s, `AS_REST`; touching the
  FM-1 stopped holds nothing, his choice; never make it sooner);
  the METER moved to MIDI's K3, EXPERT only (BEGINNER turns it off).
- Drum presets (11 Nov 2026, in progress): FLEXOR picks styled presets by ear on listening pages (rendered on the
  host); his picks, the prototype engine and the plan in `docs/sounds/PERC_PRESETS.md`. His aim: BEGINNER's drum pages
  only a PRESET and an FX, EXPERT every setting (accessibility: surf the machine first, the depth after).
  0.225: the engine's extras (perc.h PX_*: DROP, DTIME, DIRT, LIFT, ROOM; `perc_set_x`), per memory in `smem` (the
  MELODY's old place, `perc_x()`, `set.pxv` = PX_OK), FLEXOR's presets PP_SN / PP_CP in ui/perc.c with their FX,
  pages SOUND / SHAPE (K4 DIRT) / MORE, BEGINNER's PRESET / FX / LEVEL / TUNE.
- BLACK KEYS (11 Nov 2026, 0.244, FLEXOR): each black key a function (app.c `BKF_*`: PAD_* 0-10, FREEZE and DUB
  back from CHOPPAH, then BKF_ARP, KIL, DST, SUB); `bk_fn(j)`, `bkf_mask(f)` (= `PAD_BIT`) follow the layout in use:
  presets CHOPPAH (a new FM-1's and RESET ALL's; BEGINNER keeps the layout in use since 0.9.23, FLEXOR: `bk_preset` no longer forces it; DST's
  switch (`terror_off`: selk_hi bit 4 and a DST key in the layout, `bk_has`) and SUB DRIVE are each kick's own, FLEXOR 0.9.24), FLEXOR LAYOUT (FLEXOR's, 0.9.26:
  R8 R16 R32 RND HLF FLT ARP VNL KIL DST SUB; keep it exactly; on BLACK KEYS ALGORITHM moves along the keys as K2), CUSTOM; stored in `set.bkeys.bk` (the STAB's old 24 bytes: BK_TAG, preset, eleven functions; bk[15]
  BK_PLAT 0xA0 | the K4 LATCH bits of REVERSE, FREEZE, GATE, RANDOM, bk[16] BK_FTHRU 0xC0 | FILTER's K4 THRU BREAK / DRUMS /
  BOTH, 0.9.26, FLEXOR: no LATCH on FILTER; bk[17..23] free, tag new bytes: older saves hold the STAB's leftovers there). ARP's
  tabs in EXPERT (`ui.opage`, ovl.c): ARP EFFECTS, BLACK KEYS (bk_turn, draw_bk); MORE gone for now (0.259, FLEXOR; its idea: new black-key
  settings, e.g. the ARP effects one a key). SYSTEM › MIDI shows USB and TRS only (no BT box, 0.259). The simulator starts on FLEXOR LAYOUT (its scenarios).
- BREAK TIME (11 Nov 2026, 0.256, FLEXOR): HOME › SETTINGS K4 (SYSTEM › MIDI K4 until 0.9.1) NORMAL / HALF, `set.bkeys.bk[BK_BHALF]` (the STAB's old
  bytes again: bk[14] BK_VIEW since 0.9.2, bk[15], bk[16] the latches and FILTER's THRU (0.9.26), bk[17..23] still free); brk.c `p_bhalf` (`brk_half_time`): the break a step every other 16th at half
  ratio, the drums' step events, the swing and the clock at the tempo (they count `steps_total - pat_org`, not `step`).
- RESET (0.209): after a reset its page's knobs do nothing for 3 s (`RST_COOL`, `ui.rst_done`, rst_cool()), DONE shown.
  WIPE MEMORY (0.263, FLEXOR: V15 odd after LOAF BREAKER): RESET's item before EVERYTHING, erases every sector of the
  store map but the panel's (track.c `wipe_step`, a sector a frame, plat_data_erase), settings unreadable, `wiped`
  blocks saves until a restart; on the host the store is kept apart, so only the FM-1 really erases.
- No Bluetooth, ever (FLEXOR's rule). The GLOUPI BREAKS start-up photo must never come back into the repository. FLEXOR's
  sleeping photo (art/egg_sleep.png, commit afbbd5c, 6 Nov) stays in the history: FLEXOR, 12 Nov 2026, no purge.
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
  0.192 with a 357 KB bank; 561,424 B at 0.220 (9 breaks); 568,492 B at 0.225 (the drums' extras): **~13 KB left**. **580,660 B at 0.9.26 (the latches and FILTER's THRU): ~0.9 KB left; 581,312 B at 0.9.30 (GLO EFFECTS' cards): 252 B left; then AMEN15 174 out (FLEXOR): 554,144 B with the drums' 64 steps and VINYL BREAK's platter (its face 2 KB, app/face_data.h from tools/make_face.py): ~27 KB left; RAM 96,396 of 98,304; .ram_text 17,200 of 32,768. 556,928 B at 0.9.32 (the player's breaks in two zones, TERROR and SPINE): ~24.6 KB left; 557,160 B with KICK 7-8 TERROR, SPINE; 573,840 B at 0.9.34 with FLEXOR's three breaks (9 breaks): ~7.7 KB left; RAM 96,508; .ram_text 17,400.**
  RAM .data+.bss 93,292 of 98,304 B at 0.225 (the drums' room, 5.8 KB of 16-bit lines, is in RAM): ~5 KB left; the
  pool has ~10.8 KB (it could take the room's lines). The break bank (`CH_BANK`) is most of it.
  New features are welcome before 1.0 (FLEXOR, 11 Nov 2026: "not that close to 1.0, I want to perfect the firmware");
  anything big needs something taken out first (ask FLEXOR what).
  Block-rate engine code is `BRK_COLD` (built for size) and calls the shared maths (`brk_exp2f_c` and the like,
  brk_math.c; the UI too) instead of an inline copy; the FX chain's per-sample loop (`fx_samples`) is `BRK_RAM`:
  .ram_text on the FM-1 (fast, packed in flash), so nothing it reaches may be a call (`BRK_INL` in brk_fx.c forces
  inlining; the build fails on "ram_text contains calls"; flash code reaches it through a pointer). CPU: the render
  must stay under its 5.8 ms block, or audio glitches and the watchdog resets the FM-1 (0.9.26: brk_fx.c's slot_run works out
  one side while the two are the same, `mono`; an effect that makes them differ must be in `FX_STEREO`); `APP_OVERLOAD` sheds FX slots
  as a last resort (PRD E-7). Compare an engine change's cost on the host against the last release before shipping (`APP_SYS_TEXT` in app.h draws the platform's screens in CHOPPAH's faces,
  saving fm1-base's fonts): check `./build.sh`'s image line;
  the canvas is 240×60 (`APP_CANVAS_PX`), so draw in strips of ≤ 60 rows (draw_fx / draw_key draw two bands).
- Breaks: `breaks/breaks.txt` (paths relative to `breaks/`) marks with `*` the WAVs built into the firmware;
  `./build.sh` regenerates `app/bank_data.h` (never commit it, never edit it). The bank has 400 KB (`IMAGE_CAPACITY`
  in `tools/prep_breaks.py`) and 16 slots, a break at most `CH_PCM_MAX` 106,496 samples (pool ~326 of 344 KB). The breaks are the bank's only: brk_synth is the no-bank fallback (and
  the tests'). The player's own breaks (0.9.32, FLEXOR, from CHOPPAH 1.4's custom break): two banks (zones, `CU_ZB`, `CU_MAX` 4 each, FLEXOR 14 Nov 2026): zone 0 0x93000 (XDATA0 + the data area up to OBJ_KLAB, 35 sectors, 140 KB), zone 1 0xE5000 (XDATA1, 23 sectors, 92 KB), five to seven breaks; in XDATA never "AT" at a sector's 0xF06 (the update loader's record: the page's `atFix`, the FM-1's `cu_at_ok`); a 0.9.32 TEST's bank at 0x97000 (`CU_OLD`) read until zone 0 is written, written whole over USB SysEx from the site's BREAKS page (`app/site/breaks.html` -> breaks/, its library breaks/lib/ made by `tools/break_lib.py` from breaks.txt's 70 at site build; `app_sysex` in ui/main.c, the canvas as scratch); present, they come AFTER the bank's, zone 0's then zone 1's (`slot_of`, `n_user`, `n_uz`, `n_bank`: saved break numbers keep theirs); tests/custom_break.sh and tests/page_parity.sh in CI. FLEXOR: no hiding of the built-in ones. The breaks stay 2 bars (32 steps; a one-bar break plays twice, LO-FI 165 was one in 0.196-0.199).
- Releases are automatic: every push to `loaf` is a release. Numbers A.B.C since 12 Nov 2026 (FLEXOR: "on passe en
  0.9.1"): the last `loaf-vA.B.C` tag's patch + 1 (`scripts/loaf_version.sh next`), or `./LOAF_FROM` if higher
  (0.9.21 now, FLEXOR: "on va dire 0.9.21"; then 0.9.22 ...; write 0.10.0 or 1.0.0 there for a new minor or major). Before: X.YY, 0.02 to 0.267. The package's own
  number (fm1-base's X.Y.Z, two digits each) is `scripts/loaf_version.sh pkg`: 0.267 is 1.0.68, A.B.C is A+2 . B . C
  (0.9.1 is 2.9.1, 1.0.0 is 3.0.0), always above the one before (`check` in CI). Don't tag by hand. (`scripts/next_version.sh` and
  `VERSION` are CHOPPAH's main branch's.)
- **Always push to GitHub after changes** (Laurens' standing rule, 4 Nov 2026): commit and push as soon as a change is
  done, before moving on: to `dev` (a TEST build for FLEXOR), never straight to `loaf` (FLEXOR, 12 Nov 2026); `loaf`
  takes dev when FLEXOR asks for a release. Work that doesn't build or fit yet goes to a branch of its own.

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
