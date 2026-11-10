/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the controls (knobs, keys, buttons), the lights, the foot and the frame. (Part of app.c's unity:
 * included there in order.) */

/* LOAF BREAKER: KICK FX: the FX page on the kick's slots (4, 5); (TEST) EDIT KICK's FX 1, FX 2 parts */
#define IS_KFX() (ui.page == PG_FX && ui.fx_focus >= BRK_FX_SLOTS)
static void kfx_open(int on)             /* to KICK FX (on), or the FX page back on the chain */
{
    if (on && ui.fx_focus < BRK_FX_SLOTS) {
        ui.fx_main = ui.fx_focus;
        ui.fx_focus = (uint8_t)(BRK_FX_SLOTS + (ui.kfx_last % BRK_KFX_SLOTS));
        ui.fx_page = 0u;
    } else if (!on && ui.fx_focus >= BRK_FX_SLOTS) {
        ui.kfx_last = (uint8_t)(ui.fx_focus - BRK_FX_SLOTS);
        ui.fx_focus = (uint8_t)(ui.fx_main % BRK_FX_SLOTS);
        ui.fx_page = 0u;
    }
}
static uint8_t lvl_fast;                /* LOAF BREAKER: LFO + K1-K4: the levels by 5 % a detent (the MIXER's own
                                          * page: 2 %) */
static float lvl_step(int32_t d) { (void)d; return lvl_fast ? 0.05f : 0.02f; }
static void knob_turn(int k, int32_t d)
{
    brk_fx_t *f = &set.fx;
    if (ui.page == PG_TRACK || RST_ON) {   /* LOAF BREAKER: ENV's TRACKS, PLAY, DROP, BUILD; GLO's RESET */
        track_turn(k - EN_K1, d);
        return;
    }
    if (ui.page == PG_GLOBAL) {
        global_turn(k - EN_K1, d);
        return;
    }
    if (ui.page == PG_KICK) {
        kick_knob_turn(k - EN_K1, d);
        return;
    }
    if (ui.page == PG_SEQ) {
        seq_turn(k - EN_K1, d);
        return;
    }
    if (ui.page == PG_PERC) {
        perc_turn(k - EN_K1, d);
        return;
    }
    if (ui.page == PG_KEQ) {             /* LOAF BREAKER: HOME's KICK EQ */
        keq_turn(k - EN_K1, d);
        return;
    }
    if (ui.page == PG_MIX) {             /* LEVELS: break, kick, snare and hat, MELODY; SENDS: FX, the drums' route (LOAF BREAKER 0.201) */
        char t[16];
        if (ui.mpage == 3u) {            /* DYNAMICS: COMP's attack and release, DUCK's release, what it ducks */
            if (k == EN_K1 || k == EN_K2)
                selk_turn(2u, k == EN_K1 ? 2 : 3, d);
            else
                selk_turn(0u, k == EN_K3 ? 2 : 3, d);
        } else if (ui.mpage == 2u) {     /* MASTER: COMP, its RATIO, DUCK, MAX OUT */
            if (k == EN_K1 || k == EN_K2)
                selk_turn(2u, k - EN_K1, d);
            else if (k == EN_K3)
                selk_turn(0u, 0, d);
            else {
                set.max_out = (uint8_t)(clamp((int32_t)max_out_db() - (d > 0 ? 1 : -1), 0, 12) + 1);
                mix_value(2u, 3, t);
                app_say("MAX OUT", t);
            }
        } else if (ui.mpage) {           /* LOAF BREAKER: SENDS: K1 FX (the chain against the dry), K2 the drums' route
                                          * (SWING: PATTERN's GROOVE) */
            if (k == EN_K1) {
                f->wet = knob_f(f->wet, d, lvl_step(d), 0.0f, 1.0f);
                say_int("FX", (int32_t)(f->wet * 100.0f + 0.5f), "%");
            } else if (k == EN_K2) {
                set.drums_chain = d > 0 ? 0u : 1u;   /* right: around the chain, left: through it */
                app_say("DRUMS", set.drums_chain ? "THRU FX" : "AROUND FX");
            } else
                return;
        } else if (k == EN_K1) {
            set.brk_level = (uint8_t)clamp((int32_t)set.brk_level + d * (int32_t)(lvl_step(d) * 100.0f + 0.5f), 0, 100);
            say_int("BREAK", set.brk_level, "%");
        } else if (k == EN_K2) {
            set.kick.volume = knob_f(set.kick.volume, d, lvl_step(d), 0.0f, 1.0f);
            pct_text(set.kick.volume, t);
            app_say("KICK", t);
        } else if (k == EN_K3) {         /* PERC: the snare and the hat */
            set.perc.volume = knob_f(set.perc.volume, d, lvl_step(d), 0.0f, 1.0f);
            say_int("PERC", (int32_t)(set.perc.volume * 100.0f + 0.5f), "%");
        } else {                         /* LOAF BREAKER: MELO, the MELODY's own fader (FX: SENDS' K1) */
            set.mel_vol = (uint8_t)clamp((int32_t)set.mel_vol + d * (int32_t)(lvl_step(d) * 100.0f + 0.5f), 0, 100);
            say_int("MELODY", set.mel_vol, "%");
        }
        ui.redraw |= RG_BLOCKS;
    } else if (ui.page == PG_HOME) {
        if (k == EN_K1) {                /* the gate: how much of every slice sounds (complexity: GENERATOR) */
            set.gate = (uint8_t)clamp((int32_t)(set.gate ? set.gate : 100u) + d * 5, 5, 100);
            if (set.gate >= 100u)
                app_say("GATE", "OFF");
            else
                say_int("GATE", set.gate, "%");
        } else if (k == EN_K2) {         /* the break's DJ filter (LOAF BREAKER: a low-pass alone) */
            brk_flt_turn(d);             /* (LOAF BREAKER: a low-pass alone, pads.c) */
        } else if (k == EN_K3) {         /* GLOUPI BREAKS: the kick's own DJ filter (CHOPPAH: FX) */
            kick_flt_turn(d);            /* (LOAF BREAKER: SEQ's K2 the same) */
        } else {                         /* GLOUPI BREAKS: the kick's SUB decay (EDIT's; CHOPPAH: COMPLEXITY, on
                                          * GENERATOR alone now) */
            kick_layer_t *l = &set.kick.l[KL_SUB];
            l->decay = knob_f(l->decay, d, 0.02f, 0.0f, 1.0f);
            say_int("SUB DECAY", (int32_t)(kick_decay_ms(KL_SUB, l->decay) + 0.5f), " MS");
        }
    } else if (ui.page == PG_GEN) {
        float *p = k == EN_K1 ? &set.gen.complexity : k == EN_K2 ? &set.gen.anchor : k == EN_K3 ? &set.gen.roll
                 : &set.gen.fill;
        static const char *const N[4] = {"COMPLEXITY", "ANCHOR", "ROLL", "FILL"};
        *p = knob_f(*p, d, 0.02f, 0.0f, 1.0f);
        say_int(N[k - EN_K1], (int32_t)(*p * 100.0f), "%");
    } else {                             /* FX: the focused slot's three settings and its mix */
        fx_knob_turn(ui.fx_focus, k - EN_K1, d);
        return;
    }
    changed();
}

/* EDIT: the drum ui.einst's sound (the kick's page, or the snare's or the hat's) */
static void edit_open(void)
{
    ui.page = ui.einst ? PG_PERC : PG_KICK;
    ui.redraw |= RG_FOOT | RG_BLOCKS;
    if (ui.einst)
        ui.pvoice = (uint8_t)(ui.einst - 1u);
    page_say();
}

/* a break change (ALGORITHM): its name at once, the break itself at the next bar. Playing, break_poll waits for
 * the bar's last 16th, mutes the break (it runs on, the clock and the drums too), decodes the new one over pcm_a
 * once the fade is done, and the render swaps it in on the next step: the bar's first. Stopped: at once */
static void break_ask(uint32_t i)
{
    set.brk = (uint8_t)i;                /* turning on fast just moves the target */
    brk_next = (int32_t)i;
    set_dirty = 1u;
    app_say(playing_now() ? "NEXT BREAK" : "BREAK", break_name(i));
    ui.redraw |= RG_HEAD;
}

/* GLOBAL's RESET ALL: every setting and pattern to the start, saved (the Bluetooth pairings stay: OBJ_BLE) */
static void reset_all(void)
{
    uint8_t ble_was_off = set.ble_off;
    settings_defaults();
    klab_defaults();                     /* (LOAF BREAKER: the KICK LAB off on every kick) */
    kx_all();                            /* (every kick's knobs as they start) */
    if (n_breaks > n_synth)
        set.brk = (uint8_t)n_synth;      /* the bank's first break */
    pat_defaults();
    if (ble_was_off)
        plat_ble_enable(1);              /* (Bluetooth MIDI on again, as at the start) */
    break_ask(set.brk);
    settings_apply();
    pat_apply();
    pat_next = -1;
    {                                    /* (LOAF BREAKER: the four groups of patterns empty, P1-16 in) */
        uint32_t g;
        for (g = NGRP; g-- > 0u;) {
            set.pgrp = (uint8_t)g;
            (void)grp_save();
        }
    }
    (void)store_all();
    set_dirty = 0u;
    app_say("ALL CLEARED", "");
    ui.lobo_t = plat_ms() | 1u;          /* the lobotomy screen (app_frame) */
    draw_lobotomy();
}
static void break_poll(void)
{
    static uint32_t mark;
    static uint8_t muting;
    if (brk_next < 0)
        return;
    if (!muting) {
        brk_status_t st;
        brk_status(&st);
        if (st.playing && st.bar16 >= 0 && st.bar16 != 15)
            return;                      /* playing: not before the bar's last 16th */
        brk_mute(1);
        mark = brk_renders();
        muting = 1u;
        return;
    }
    if (brk_renders() - mark < 2u)       /* the fade out done */
        return;
    brk_swap_break(break_load((uint32_t)brk_next));
    brk_next = -1;
    muting = 0u;
    ui.redraw |= RG_ALL;
}

/* white keys held into a page where they don't play slices: let the engine's loops go */
static void slices_let_go(uint32_t keys)
{
    uint32_t i;
    if (!slices_page())
        return;
    for (i = 0; i < 16u; i++)
        if (keys >> WK[i] & 1u)
            brk_key((int)i, 0);
}

static uint32_t seq_t;                   /* LOAF BREAKER: SEQ pressed: when, and waiting (1, 3: and the next lane)
                                          * for its release */
static uint8_t seq_wait;
static uint32_t fxl_t;                   /* LOAF BREAKER: FX pressed: when (held 0.7 s: the FX fader) */
static uint32_t clr_t;                   /* LOAF BREAKER: SEQ held: the clear asked, waiting for SEQ again (3 s) */
static uint8_t clr_arm;
static uint32_t rec_t;                   /* LOAF BREAKER: REC pressed on SEQ: when, and waiting for its release */
static uint8_t rec_wait;
static void rec_toggle(void)
{
    ui.rec ^= 1u;
    rec_step = 0u;                       /* (stopped: step by step from step 1) */
    app_say("REC", ui.rec ? (playing_now() ? "ON" : ui.page == PG_SEQ && ui.piano ? "ON  KEYS WRITE" : "ON  STEP 1") : "OFF");
    ui.redraw |= RG_HEAD | RG_BLOCKS;
}

static void input(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), now = plat_ms(), i, sel_now = btn >> B_SEL & 1u;
    uint32_t kdn, kup, bdn;
    if (ui.sel_lock) {                   /* LOAF BREAKER: SEL locked (SEL x2): it counts as held, lit, until
                                          * pressed again (that press no click) */
        if (sel_now && !ui.sel_raw) {
            ui.sel_lock = 0u;
            app_say("SEL", "FREE");
        } else
            btn |= 1u << B_SEL;
    }
    ui.sel_raw = (uint8_t)sel_now;
    kdn = keys & ~ui.keys;
    kup = ui.keys & ~keys;
    bdn = btn & ~ui.btn;
    int k;
    if (keys != ui.keys || btn != ui.btn)
        ui.touched = 1u;                 /* (read here too between frames: app_input_only) */
    if (ui.lobo_t || demo.on) {          /* RESET ALL's screen or the demo up: a press only ends it (app_frame) */
        for (i = 0; i < 16u; i++)        /* (but what is let go is let go: no key loop or pad left on) */
            if (kup >> WK[i] & 1u)
                brk_key((int)i, 0);
        for (i = 0; i < 11u; i++)
            if ((kup & PAD_BIT(i)))
                brk_pad((int)i, 0);
        for (k = 0; k < NE; k++)
            if (plat_enc(k))
                ui.touched = 1u;
        ui.home_hold = ui.fx_hold = ui.tap_pend = ui.env_hold = ui.lfo_hold = 0u;
        ui.keys = keys;
        ui.btn = btn;
        return;
    }
    for (i = 0; i < 16u; i++) {          /* white keys: slices, or a page's own */
        if (i < NDRUMS && (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() || ui.page == PG_PERC) &&
            !playing_now()) {            /* LOAF BREAKER: the KICK menu (big or classic, its EQ and FX parts too) and the
                                          * drums' pages (big or classic), stopped: keys 1-5 play the KICK, SNARE, HAT,
                                          * CLAP, HOOVER once each (to hear them); playing, they play slices as ever */
            if (kdn >> WK[i] & 1u) {
                midi_drum(i);
                if (!(ui.page == PG_KICK && ui.kclassic))   /* (where no card lights: its name on the strip) */
                    app_say("PLAY", DRUM_NAME[i]);
                ui.aud_t = now;
                ui.aud_d = (uint8_t)i;
                ui.redraw |= RG_BLOCKS;
            }
            continue;
        }
        if (ui.page == PG_GLOBAL && ui.gpage == 3u) {   /* LOAF BREAKER: GLO EFFECTS: keys 1-7 each an effect on or off */
            if (i < 7u && kdn >> WK[i] & 1u)
                sfx_toggle(i + 1u, -1);
            continue;
        }
        if (ui.page == PG_TRACK) {       /* LOAF BREAKER: ENV: a pattern into the track, or a track played */
            if (kdn >> WK[i] & 1u)
                track_key(i);
            continue;
        }
        if (ui.page == PG_SEQ) {         /* the SEQ page: a break step to edit, or a drum lane's steps */
            if (ui.piano && !(btn & 1u << B_SEQ)) {   /* LOAF BREAKER: HV KEYS: a note of the hoover */
                if (kdn >> WK[i] & 1u)
                    piano_key(PIANO_W[i]);
                continue;
            }
            if (btn & 1u << B_SEQ) {     /* LOAF BREAKER: SEQ held + white key 1..4: the bar shown copied there */
                if (kdn >> WK[i] & 1u) {
                    if (i < 4u)
                        seq_copy_bar(i);
                    seq_wait = 0u;       /* (no next lane at SEQ's release, no CLEAR armed) */
                    clr_arm = 0u;
                }
                continue;
            }
            if (ui.rec && sl_notes(ui.seq_lane) && playing_now()) {   /* LOAF BREAKER: REC, playing, a drum lane:
                                          * any white key plays its drum and writes it at the nearest 16th */
                if (kdn >> WK[i] & 1u)
                    midi_drum(sl_drum(ui.seq_lane));
                continue;
            }
            if (sl_notes(ui.seq_lane)) { /* LOAF BREAKER: a drum lane's key acts as it comes up, unless a knob
                                          * turned meanwhile (then that step's own: NOTE, HITS, CHANCE, NUDGE; the foot
                                          * names them while it is held) */
                if (kdn >> WK[i] & 1u) {
                    ui.note_key = (uint8_t)(i + 1u);
                    ui.note_turned = 0u;
                    ui.redraw |= RG_FOOT;
                }
                if (kup >> WK[i] & 1u && ui.note_key == i + 1u) {
                    if (!ui.note_turned)
                        seq_key(i);
                    ui.note_key = 0u;
                    ui.redraw |= RG_FOOT;
                }
                continue;
            }
            if (kdn >> WK[i] & 1u)
                seq_key(i);
            continue;
        }
        if (kdn >> WK[i] & 1u) {
            if (ui.rec)
                rec_slice(i);            /* REC: into the pattern too (stopped: step by step, LOAF BREAKER) */
            brk_key((int)i, 1);
            ui.key_focus = (int8_t)i;    /* the engine loops the key pressed last */
            ui.key_since = now;
            ui.key_turned = 0u;
        }
        if (kup >> WK[i] & 1u)
            brk_key((int)i, 0);
    }
    if (ui.key_focus >= 0 && (!slices_page() || !(keys >> WK[ui.key_focus] & 1u))) {
        ui.key_focus = -1;               /* let go: the lowest key still held loops (as in the engine), or none */
        if (slices_page())
            for (i = 16u; i-- > 0u;)
                if (keys >> WK[i] & 1u) {
                    ui.key_focus = (int8_t)i;
                    ui.key_since = now;
                }
    }
    if (ui.page == PG_SEQ && ui.piano) { /* LOAF BREAKER: HV KEYS: the black keys notes too (left to right) */
        for (i = 0; i < 11u; i++)
            if (kdn >> BK[i] & 1u)
                piano_key(PIANO_B[i]);
    } else
    for (i = 0; i < 11u; i++) {          /* black keys: the pads, held; the last one pressed has the screen */
        if (i == PAD_FREEZE) {           /* GLOUPI BREAKS: FREEZE's key turns the kick's SUB DRIVE on and off */
            if ((kdn & PAD_BIT(i)))
                sub_drive_toggle();
            continue;
        }
        if (i == PAD_DUB) {              /* GLOUPI BREAKS: DUB's is the ARP key, an OVERLAY effect (ovl.c) */
            if ((kdn & PAD_BIT(i))) {
                ovl_pad_key(1);
                ui.pad_focus = (int8_t)i;
                ui.pad_since = now;
            }
            if ((kup & PAD_BIT(i)))
                ovl_pad_key(0);
            continue;
        }
        if (i <= PAD_ROLL32) {           /* LOAF BREAKER: the rolls, through their THRU (the break, the kick, both) */
            if ((kdn & PAD_BIT(i))) {
                roll_key(i, 1, (btn & 1u << B_SEL) != 0u);   /* (SEL + the pad: the kick alone, this press) */
                if (btn & 1u << B_SEL)
                    ui.tap_combo = 1u;   /* (no SEL click) */
                ui.pad_focus = (int8_t)i;
                ui.pad_since = now;
            }
            if ((kup & PAD_BIT(i)))
                roll_key(i, 0, 0);
            continue;
        }
        if (i == PAD_HALF && set.half_mt_off & 2u) {   /* LOAF BREAKER: HALF SPEED's LATCH: a press on, a press
                                          * off (its knobs while held, as ever) */
            if ((kdn & PAD_BIT(i))) {
                half_lat ^= 1u;
                brk_pad((int)i, half_lat);
                ui.pad_focus = (int8_t)i;
                ui.pad_since = now;
                app_say("HALF SPEED", half_lat ? "LATCHED" : "OFF");
            }
            continue;
        }
        if (i == PAD_HALF && half_lat && !((keys & PAD_BIT(i)))) {   /* (LATCH turned off, let go: off) */
            half_lat = 0u;
            brk_pad((int)i, 0);
            continue;
        }
        if ((kdn & PAD_BIT(i))) {
            brk_pad((int)i, 1);
            ui.pad_focus = (int8_t)i;
            ui.pad_since = now;
        }
        if ((kup & PAD_BIT(i)) && !(i == PAD_HALF && half_lat))
            brk_pad((int)i, 0);
    }
    if (kdn >> BK[TERR_KEY] & 1u) {      /* LOAF BREAKER: TER, as SUB's DRIVE: the kick's TERROR off if it sounds (its
                                          * amount kept), else on (an amount at nothing: 50 %, a key that does something) */
        if (KICK_TERROR(&set.kick) && !terror_off)
            set.selk_hi |= 16u;
        else {
            set.selk_hi &= (uint8_t)~16u;
            if (!KICK_TERROR(&set.kick))
                KICK_TERROR(&set.kick) = 50u;
        }
        app_say("DISTO", terror_off ? "OFF" : "ON");
        changed_kick();
        ui.redraw |= RG_FOOT | RG_BLOCKS;
    }
    if (kdn >> BK[KILL_KEY] & 1u) {      /* LOAF BREAKER: KIL held: the sequencer's kicks quiet; let go: back. Its
                                          * LATCH (K1 while held): a press quiet, the next back */
        ui.kil_focus = 1u;
        kick_hold = set.half_mt_off & 4u ? (uint8_t)!kick_hold : 1u;
        app_say("KICKS", kick_hold ? (set.half_mt_off & 4u ? "QUIET (KIL LATCHED)" : "QUIET (KIL)") : "BACK");
        ui.redraw |= RG_FOOT;
    } else if (!(keys >> BK[KILL_KEY] & 1u)) {
        if (ui.kil_focus) {
            ui.kil_focus = 0u;
            ui.redraw |= RG_FOOT;
        }
        if (kick_hold && !(set.half_mt_off & 4u)) {   /* (let go, no LATCH: back) */
            kick_hold = 0u;
            app_say("KICKS", "BACK");
            ui.redraw |= RG_FOOT;
        }
    }
    if (ui.pad_focus >= 0 && !((keys & PAD_BIT(ui.pad_focus)))) {   /* let go: another pad still held, or none */
        ui.pad_focus = -1;
        for (i = 0; i < 11u; i++)
            if (i != PAD_FREEZE && (keys & PAD_BIT(i)))
                ui.pad_focus = (int8_t)i;
    }
    if (bdn & 1u << B_PLAY) {
        brk_status_t st;
        brk_status(&st);
        if (btn & 1u << B_HOME) {        /* HOME + PLAY: back to bar 1 */
            transport(st.playing, 1);
            app_say("RESET", "");
            ui.home_combo = 1u;          /* (not HOME's next part) */
        } else if (ui.page == PG_TRACK && ui.tpage == 0u && btn & 1u << B_SEL) {
            track_from_here();           /* LOAF BREAKER: TRACKS, SEL + PLAY: PLAY starts the track there */
            ui.tap_combo = 1u;           /* (no SEL click) */
        } else if (ui.page == PG_TRACK && ui.tpage <= 1u && !(st.playing && ui.trk_on))
            track_play(ui.tpage == 1u);  /* LOAF BREAKER: ENV's TRACKS or PLAY: the track from its first item (PLAY:
                                          * then HOME; TRACKS: it stays) */
        else
            transport(!st.playing, 0);
        muted = 0u;
    }
    if (bdn & 1u << B_HOME)
        ui.home_t = now;                 /* (LOAF BREAKER: held 0.35 s on PERFORM / GENERATOR: the CDJ view) */
    if (bdn & 1u << B_HOME) {            /* HOME: PERFORM; again (let go alone), GENERATOR, and back */
        if (ui.page == PG_HOME || ui.page == PG_GEN) {   /* (TEST: HOME is PERFORM and GENERATOR) */
            ui.home_hold = 1u;
            ui.home_combo = 0u;
        } else {
            ui.page = PG_HOME;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (ui.home_hold && !(btn & 1u << B_HOME)) {
        ui.home_hold = 0u;
        if (!ui.home_combo && (ui.page == PG_HOME || ui.page == PG_GEN)) {
            ui.page = ui.page == PG_HOME ? PG_GEN : PG_HOME;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (bdn & 1u << B_GLO)               /* GLOBAL: MIDI clock and Bluetooth; again, the pitch; again, BT audio */
        global_part(glo_next(1));        /* (LOAF BREAKER: SYSTEM's parts: MIDI, SETUP, EFFECTS, FX SET, RESET) */
    if (bdn & 1u << B_EDIT) {            /* LOAF BREAKER: EDIT: SOUNDS, the KICK menu; again, its next section (SOUND
                                          * SYNTH DIRT TONE KNOBS). SEL + EDIT: the next source: KICK, DRUMS, MELODY,
                                          * BREAK (0.201) */
        int kickp = ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX();
        if (btn & 1u << B_SEL) {
            ui.pbig = (uint8_t)!ui.kclassic;   /* (the drums in the view the kick has) */
            if (kickp) {
                ui.einst = DR_SNARE;
                edit_open();
            } else if (ui.page == PG_PERC && ui.einst != DR_STAB) {
                ui.einst = DR_STAB;
                edit_open();
            } else if (ui.page == PG_PERC)
                global_part(GP_BREAK);
            else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK) {
                ui.einst = 0u;
                kick_open();
            } else {
                ui.einst = (uint8_t)(ui.einst >= DR_SNARE && ui.einst < DR_STAB ? ui.einst : DR_SNARE);
                edit_open();
            }
        } else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK)
            app_say("BREAK", "ONE PART");    /* (the break's page: one part; SEL + EDIT the kick) */
        else if (kickp || ui.page == PG_PERC) {
            ui.edit_hold = 1u;           /* (at its release: the next section, the next drum; held: the other view) */
            ui.edit_t = now;
        } else if (ui.page == PG_SEQ && ui.seq_lane >= SL_KICK && sl_drum(ui.seq_lane)) {
            ui.einst = (uint8_t)sl_drum(ui.seq_lane);   /* (from SEQ, the drum of the lane shown) */
            edit_open();
        } else {
            ui.einst = 0u;
            kick_open();
        }
        ui.redraw |= RG_FOOT | RG_BLOCKS | RG_WAVE;
    }
    if (ui.edit_hold && btn & 1u << B_EDIT && now - ui.edit_t >= 450u && ui.edit_hold == 1u) {   /* LOAF BREAKER: EDIT
                                          * held on the KICK menu: the view as it was (the drums' shapes), or back */
        ui.edit_hold = 2u;
        if (ui.page == PG_KICK && !ui.kclassic && ui.kview == 0u) {   /* (LOAF BREAKER, 0.205: the KICK menu's views,
                                          * RGB, WAVE, CLASSIC, in turn) */
            ui.kview = 1u;
            kv_sig = 0u;
        } else {
            ui.kclassic ^= 1u;           /* (LOAF BREAKER: on the drums' pages too: one view for them all) */
            if (ui.page == PG_PERC)
                ui.kclassic = ui.pbig;
            ui.pbig = (uint8_t)!ui.kclassic;
            if (!ui.kclassic && ui.page == PG_KICK) {
                ui.kview = 0u;
                kv_sig = 0u;
            }
        }
        app_say(ui.page == PG_PERC ? PERC_VOICE_NAME[ui.pvoice % PERC_VOICES] : "KICK",
                ui.kclassic ? "CLASSIC VIEW" : ui.page != PG_KICK ? "BIG VIEW" : ui.kview ? "WAVE VIEW" : "RGB VIEW");
        ui.redraw |= RG_BLOCKS | RG_WAVE | RG_FOOT;
        if (!ui.kclassic)
            lcd_fill(0, 44, 240, 44, C_BLACK);
    }
    if (ui.edit_hold && !(btn & 1u << B_EDIT)) {
        if (ui.edit_hold == 1u && ui.page == PG_PERC && ui.einst == DR_STAB) {   /* (LOAF BREAKER: let go before the
                                          * hold: the MELODY's next part, SOUND or SHAPE) */
            ui.ppage ^= 1u;
            page_say();
        } else if (ui.edit_hold == 1u && ui.page == PG_PERC) {   /* (on a drum: the next one, SNARE HAT CLAP round) */
            ui.einst = (uint8_t)(ui.einst >= DR_CLAP ? DR_SNARE : ui.einst + 1u);
            edit_open();
        } else if (ui.edit_hold == 1u)
            kick_sect_next();            /* (let go before the hold: the next section) */
        ui.edit_hold = 0u;
        ui.redraw |= RG_FOOT | RG_BLOCKS;
    }
    if (bdn & 1u << B_LFO)               /* LOAF BREAKER: LFO acts as it comes up, unless K1-K4 turned meanwhile (LFO +
                                          * K1-K4: the four levels, on any page) */
        ui.lfo_hold = 1u;
    if (ui.lfo_hold && !(btn & 1u << B_LFO)) {   /* LFO: the MIXER (the break, the kick, FX main, FX overlay); again,
                                          * groove */
        if (ui.lfo_hold == 1u) {
            ui.mpage = ui.page == PG_MIX ? (uint8_t)((ui.mpage + 1u) % (set.novice ? 2u : 4u)) : 0u;   /* (LEVELS,
                                          * SENDS, MASTER, DYNAMICS; LOAF BREAKER: BEGINNER LEVELS and SENDS alone) */
            ui.page = PG_MIX;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
        ui.lfo_hold = 0u;
    }
    if (bdn & 1u << B_ENV && btn & 1u << B_SEL) {   /* LOAF BREAKER: SEL + ENV: the drop (its build from the next
                                          * bar; was SEL x3); the page stays */
        build_click();
        ui.tap_combo = 1u;               /* (no SEL click) */
    } else if (bdn & 1u << B_ENV) {      /* LOAF BREAKER: ENV: TRACKS; again, PLAY, DROP (TAP TEMPO gone). On TRACKS:
                                          * at its release; held 0.45 s, a gap after the item edited */
        if (ui.page == PG_TRACK && ui.tpage == 0u) {
            ui.env_hold = 1u;
            ui.env_t = now;
        } else {
            if (ui.page != PG_TRACK)
                slices_let_go(keys);
            track_open();
        }
    }
    if (ui.env_hold == 1u && btn & 1u << B_ENV && now - ui.env_t >= 450u) {
        ui.env_hold = 2u;
        track_gap();
    }
    if (ui.env_hold && !(btn & 1u << B_ENV)) {
        if (ui.env_hold == 1u)
            track_open();
        ui.env_hold = 0u;
    }
    if (clr_arm && (now - clr_t >= 3000u || bdn & ~(1u << B_SEQ) || kdn)) {   /* LOAF BREAKER: not confirmed */
        clr_arm = 0u;
        app_say("CLEAR", "CANCELLED");
    }
    if (bdn & 1u << B_SEQ && clr_arm) {  /* LOAF BREAKER: SEQ again: the clear confirmed (SEL + SAVE takes it back) */
        clr_arm = 0u;
        seq_clear(0);
    } else if (bdn & 1u << B_SEQ && btn & 1u << B_SEL)   /* LOAF BREAKER: SEL + SEQ: copy the pattern; on another:
                                          * paste */
        pat_copy_paste();
    else if (bdn & 1u << B_SEQ) {        /* SEQ: the pattern's steps and lanes; again, the next lane (LOAF BREAKER:
                                          * at its release; held 0.7 s, anywhere: the whole pattern cleared) */
        seq_t = now;
        seq_wait = (uint8_t)(ui.page == PG_SEQ ? 3u : 1u);
        if (ui.page != PG_SEQ) {
            slices_let_go(keys);
            ui.seq_lane = SL_KICK;       /* LOAF BREAKER: back on SEQ from another page, always K1 */
            ui.page = PG_SEQ;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (seq_wait) {
        if (!(btn & 1u << B_SEQ)) {      /* let go: on SEQ already, the next lane */
            if (seq_wait & 2u) {
                ui.seq_manual = 1u;      /* (a lane picked: the bar playing no longer followed, until stopped) */
                seq_lane_step(1);
                ui.redraw |= RG_FOOT | RG_BLOCKS;
                page_say();
            }
            seq_wait = 0u;
        } else if (now - seq_t >= 700u) {   /* held: the pattern playing, all of it, once SEQ is pressed again */
            char t[12] = "CLEAR P";
            seq_wait = 0u;
            clr_arm = 1u;
            clr_t = now;
            int_text((int32_t)pat_cur() + 1, t + 7);
            text_cat(t, "?");
            app_say(t, "SEQ AGAIN");
        }
    }
    if (bdn & 1u << B_REC && btn & 1u << B_SEQ && ui.page == PG_SEQ) {   /* LOAF BREAKER: SEQ held + REC: every drum
                                          * lane cleared, the break kept */
        seq_clear_drums();
        seq_wait = 0u;
        clr_arm = 0u;
    } else if (bdn & 1u << B_REC && btn & 1u << B_SEL && ui.page == PG_FX)
        fx_reset();                      /* LOAF BREAKER: SEL + REC on FX: the slot's effect reset (again: undone; was
                                          * SEL x2) */
    else if (bdn & 1u << B_REC && btn & 1u << B_SEL && ui.page == PG_KICK && ui.kpage == KP_FX)
        kfx_reset();                     /* (EDIT KICK's FX: the layer's effect) */
    else if (bdn & 1u << B_REC && btn & 1u << B_SEL) {   /* SEL + REC: clear (on SEQ its lane; else the pattern) */
        seq_clear(ui.page == PG_SEQ);
    } else if (bdn & 1u << B_REC) {      /* REC: record the keys and MIDI notes into the pattern while playing */
        rec_t = now;                     /* (LOAF BREAKER: on SEQ, at its release: held, it clears the break) */
        rec_wait = ui.page == PG_SEQ;
        if (!rec_wait)
            rec_toggle();
    }
    if (rec_wait) {                      /* LOAF BREAKER: REC on SEQ: let go, REC on or off; held, the break's steps
                                          * cleared (both halves; SEL + SAVE takes it back) */
        if (!(btn & 1u << B_REC)) {
            rec_wait = 0u;
            rec_toggle();
        } else if (now - rec_t >= 700u) {
            rec_wait = 0u;
            seq_clear(2);
        }
    }
    if (bdn & 1u << B_ARP && btn & 1u << B_SEL) {   /* LOAF BREAKER: SEL + ARP: FILL's page (its settings; was a
                                          * random pattern: the generator is HOME's second part) */
        global_part(GP_FILL);
    } else if (bdn & 1u << B_ARP) {      /* LOAF BREAKER TEST: ARP held: FILL (the steps set FILL play, NOT FILL
                                          * ones rest, the snare rolls the bar's last beat: FILL's page); LATCH: a
                                          * press on, the next off */
        fill_on = FILL_LATCH ? (uint8_t)!fill_on : 1u;
        app_say("FILL", FILL_LATCH ? (fill_on ? "ON" : "OFF") : "");
        ui.redraw |= RG_HEAD | (ui.page == PG_GLOBAL ? RG_BLOCKS : 0u);
    }
    if (fill_on && !FILL_LATCH && !(btn & 1u << B_ARP)) {
        fill_on = 0u;
        ui.redraw |= RG_HEAD;
    }
    if (bdn & 1u << B_FX && ui.pad_focus == PAD_DUB) {   /* LOAF BREAKER: FX with the ARP key held: its knobs, MAIN
                                          * <-> MORE */
        ui.pad_more ^= 1u;
        app_say("ARP KEY", ui.pad_more ? "MORE" : "MAIN");
        ui.redraw |= RG_BLOCKS;
        foot_dirty = 1u;
    } else if (bdn & 1u << B_FX && key_tuned() >= 0) {   /* FX with a key held: its knobs, loop <-> slice effect */
        ui.key_layer ^= 1u;
        ui.key_turned = 1u;
        app_say("KEY", ui.key_layer ? "SLICE FX" : "LOOP");
        foot_dirty = 1u;
    } else if (bdn & 1u << B_FX && btn & 1u << B_SEL && ui.page == PG_FX) {   /* SEL + FX: the slot bypassed */
        brk_fxslot_t *sl = fx_sl(fx_foc());
        if (sl->type == FXT_EMPTY)
            app_say("EMPTY", "PRESETS");
        else {
            sl->on ^= 1u;
            app_say(BRK_FXT_NAME[sl->type], sl->on ? "ON" : "BYPASS");
            changed();
        }
        ui.redraw |= RG_FOOT | RG_BLOCKS;
    } else if (bdn & 1u << B_FX) {       /* FX: the chain's page; again (let go alone): the next slot (LOAF BREAKER:
                                          * both at its release; held 0.7 s, the MIXER's FX fader 0 % or 100 %) */
        ui.fx_hold = 1u;
        ui.fx_turned = 0u;
        ui.fx_open = (uint8_t)(ui.page != PG_FX);   /* (KICK FX too: FX again, its next part) */
        fxl_t = now;
    }
    if (ui.fx_hold && !ui.fx_turned && btn & 1u << B_FX && now - fxl_t >= 700u) {   /* LOAF BREAKER: FX held: the
                                          * effects' fader all off, or all on */
        set.fx.wet = set.fx.wet > 0.001f ? 0.0f : 1.0f;
        changed();
        app_say("FX FADER", set.fx.wet > 0.0f ? "100%" : "0%");
        ui.fx_turned = 1u;               /* (no slot, no page at its release) */
        ui.redraw |= RG_BLOCKS | RG_FOOT;
    }
    if (ui.fx_hold && !(btn & 1u << B_FX)) {   /* FX let go: alone, the page or the next slot; with ALGORITHM turned, a
                                          * move */
        ui.fx_hold = 0u;
        if (!ui.fx_turned) {
            if (ui.fx_open) {
                kfx_open(0);
                ui.page = PG_FX;
                page_say();
                ui.redraw |= RG_FOOT | RG_BLOCKS;
            } else if (ui.page == PG_FX)
                fx_next_part();          /* (an MB COMP: its next part first) */
        }
    }
    if (bdn & 1u << B_SEL && key_tuned() >= 0) {   /* SEL while a key is held: its loop back to the defaults */
        if (ui.key_layer)
            brk_slicefx_defaults(&set.slx[key_tuned()]);
        else
            brk_keyset_defaults(&set.keys[key_tuned()]);
        ui.key_turned = 1u;
        changed();
        app_say(ui.key_layer ? "SLICE FX" : "KEY", "RESET");
    } else if (bdn & 1u << B_SEL) {      /* a press, if it comes up alone (below) */
        ui.tap_pend = 1u;
        ui.tap_t = now;
        ui.tap_combo = (uint8_t)((btn & ~(1u << B_SEL)) != 0u);   /* (another button already held: a combination) */
    }
    if (ui.tap_pend && (btn & 1u << B_SEL) && (bdn & ~(1u << B_SEL)))
        ui.tap_combo = 1u;               /* another button with SEL held */
    if (ui.help && ((bdn & ~(1u << B_SEL)) || kdn))
        ui.help = 0u;                    /* the card open: any other button or key closes it */
    if (ui.tap_pend && !(btn & 1u << B_SEL)) {   /* SEL up alone: counted (LOAF BREAKER: three or more, nothing;
                                          * the drop is SEL + ENV) */
        ui.tap_pend = 0u;
        if (!ui.tap_combo) {
            ui.sel_clicks = (uint8_t)(now - ui.sel_click_t < 400u && ui.sel_clicks < 9u ? ui.sel_clicks + 1u : 1u);
            ui.sel_click_t = now;
            if (ui.sel_clicks == 2u) {   /* LOAF BREAKER: SEL x2: locked down, lit, until pressed again */
                ui.sel_clicks = 0u;
                ui.sel_lock = 1u;
                app_say("SEL", "LOCKED: PRESS TO FREE");
            }
        } else
            ui.sel_clicks = 0u;
    }
    if (ui.sel_clicks && now - ui.sel_click_t >= 400u) {   /* one click alone: the combinations card, open or
                                          * closed */
        if (ui.sel_clicks == 1u) {
            ui.help ^= 1u;
            ui.help_t = now;
            if (ui.help)
                app_say("SEL", "COMBINATIONS");
        }                                /* (LOAF BREAKER: two: SEL locked, above) */
        ui.sel_clicks = 0u;
    }
    if (bdn & 1u << B_SAVE && btn & 1u << B_SEL)   /* SEL + SAVE: undo (again: redo) */
        undo_swap();
    else if (bdn & 1u << B_SAVE) {
        int pre = kpre_store();          /* (LOAF BREAKER: on STYLE, PRESET n picked: the knobs into it first) */
        int ok = store_all();
        if (ok)
            ab_take();                   /* (TOOLS' A/B: B is the kick as now saved) */
        app_say(pre ? KPRE_NAME[kstyle_at - KSTYLES] : "SAVE", ok ? (pre ? "STORED, SAVED" : "DONE") : "FAILED");
        set_dirty = 0u;
        ui.redraw |= RG_BLOCKS;
    }
    /* OCT- / OCT+: pitch in semitones, repeating faster while held; both: back to 0. LOAF BREAKER: REC stopped (step by
     * step), OCT+ a REST (the step kept, the next one), OCT- a step back */
    if (ui.rec && !playing_now()) {
        if (bdn & 1u << B_OCTUP)
            rec_move(1);
        if (bdn & 1u << B_OCTDN)
            rec_move(-1);
    } else if ((btn & (1u << B_OCTDN | 1u << B_OCTUP)) == (1u << B_OCTDN | 1u << B_OCTUP)) {
        if (bdn & (1u << B_OCTDN | 1u << B_OCTUP) && set.pitch) {
            set.pitch = 0;
            changed();
            app_say("PITCH", "0");
        }
    } else if (btn & (1u << B_OCTDN | 1u << B_OCTUP)) {
        int32_t dir = btn & 1u << B_OCTUP ? 1 : -1;
        if (bdn & (1u << B_OCTDN | 1u << B_OCTUP)) {
            ui.oct_t = now + 400u;
            ui.oct_rep = 0u;
        } else if (now >= ui.oct_t) {
            ui.oct_rep++;
            ui.oct_t = now + (ui.oct_rep < 4u ? 150u : 50u);
        } else
            dir = 0;
        if (dir) {
            set.pitch = (int8_t)clamp(set.pitch + dir, -24, 24);
            changed();
            say_int("PITCH", set.pitch, " ST");
        }
    }
    for (k = 0; k < NE; k++) {
        int32_t d = plat_enc(k);
        if (!d)
            continue;
        ui.touched = 1u;
        if (btn & 1u << B_SEL)
            ui.tap_combo = 1u;           /* (SEL + a knob is no tap) */
        if (ui.fx_hold)
            ui.fx_turned = 1u;           /* (FX + a knob: not the next slot) */
        if (!(btn & 1u << B_SEL))
            ui.help = 0u;                /* (a knob closes the card) */
        if (k == EN_SELECT && btn & 1u << B_SEL) {   /* LOAF BREAKER: SEL + SELECT, on any page: the drums' filter
                                          * (DRUM FLT, HOME's K3; was the kick's memories, now PRESETS on the KICK menu) */
            kick_flt_turn(d);
            continue;
        }
        if (k >= EN_K1 && k <= EN_K4 && btn & 1u << B_LFO) {   /* LOAF BREAKER: LFO + K1 the break's level, K2 the
                                          * kick's, K3 the drums', K4 the MELODY's, on any page (the MIXER's LEVELS: the page
                                          * stays), 5 % a detent */
            uint8_t pg = ui.page, mp = ui.mpage;
            ui.page = PG_MIX;
            ui.mpage = 0u;
            lvl_fast = 1u;
            knob_turn(k, d);
            lvl_fast = 0u;
            ui.page = pg;
            ui.mpage = mp;
            ui.lfo_hold = 2u;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            continue;
        }
        if (k == EN_K1 && ui.page == PG_SEQ && btn & 1u << B_SEQ) {   /* LOAF BREAKER: SEQ held + K1: the lane a
                                          * step later or earlier, round */
            seq_rotate(d);
            seq_wait = 0u;
            clr_arm = 0u;
            continue;
        }
        if (k >= EN_K1 && k <= EN_K4 && btn & 1u << B_SEL) {   /* LOAF BREAKER: SEL + K1 DUCK, K2 TERROR 2, K3 COMP
                                          * (was LIMIT), K4 TERROR, on any page: its amount (its settings: ENV's KNOB) */
            selk_turn(selk_fn((uint32_t)(k - EN_K1)), 0, d);
            continue;
        }
        if (k == EN_SELECT && ((ui.page == PG_KICK && !ui.kclassic) || (ui.page == PG_PERC && ui.pbig))) {
            ui.einst = (uint8_t)((ui.einst + (d > 0 ? 1u : NDRUMS - 1u)) % NDRUMS);   /* LOAF BREAKER: the big view:
                                          * SELECT goes from drum to drum (the KICK, SNARE, HAT, CLAP, HOOVER), each in
                                          * it (not the tempo here) */
            if (ui.einst) {
                ui.pbig = 1u;
                edit_open();
            } else
                kick_open();
            ui.redraw |= RG_BLOCKS | RG_FOOT;
            continue;
        }
        if (k == EN_SELECT && ext_active()) {
            app_say("TEMPO", "EXT CLOCK");   /* the outside clock has the tempo */
        } else if (k == EN_SELECT) {     /* tempo: 1 BPM a detent (SEL held: the kick, above) */
            set.tempo_x10 = (uint16_t)clamp((int32_t)set.tempo_x10 + d * 10, 400, 3000);
            changed();
            {
                char t[16];
                tenths_text(set.tempo_x10, t);
                app_say("BPM", t);
            }
        } else if (k == EN_ALGO) {       /* what the page edits; on HOME, ARP, MIXER (and SEL held: anywhere) the break */
            if (ui.page == PG_FX && btn & 1u << B_FX)
                fx_algo(d, 1u);          /* FX held: the slot moves along the chain */
            else if (btn & 1u << B_SEL)
                break_ask((set.brk + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks);
            else if (RST_ON)
                reset_pick(d);           /* LOAF BREAKER: GLO's RESET: the part, as K1 */
            else if (ui.page == PG_TRACK && ui.tpage == 0u)
                track_turn(-1, d);       /* LOAF BREAKER: ENV's TRACKS: the item (the break: SEL + ALGORITHM; K1 the
                                          * item's kick) */
            else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK)   /* LOAF BREAKER: SOUNDS' BREAK: the break */
                break_ask((set.brk + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks);
            else if (ui.page == PG_GLOBAL && ui.gpage == GP_FILL) {   /* PATTERN's GROOVE: back to SEQ's lanes (left:
                                          * KEYS) */
                ui.page = PG_SEQ;
                ui.seq_manual = 1u;
                if (d > 0)
                    ui.seq_lane = SL_BREAK1;
                else
                    piano_set(1);
                page_say();
                ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_GLOBAL)   /* GLOBAL: its part (LOAF BREAKER: SYSTEM's) */
                global_part(glo_next(d));
            else if (ui.page == PG_KICK) {    /* LOAF BREAKER: the KICK menu: the next part of its section */
                kick_sect_step(d);
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_SEQ) {   /* SEQ: the lane its keys edit */
                ui.seq_manual = 1u;      /* (LOAF BREAKER: picked: not following the bar playing, until stopped) */
                seq_lane_step(d);
                page_say();
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_KEQ || IS_KFX()) {   /* (the KICK menu's EQ and FX parts: its section's next) */
                kick_sect_step(d);
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_PERC) {   /* the snare or the hat: SOUND or SHAPE */
                ui.ppage ^= 1u;
                page_say();
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_FX)
                fx_algo(d, 0u);
            else {                     /* the next or previous break */
                break_ask((set.brk + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks);
            }
        } else if (k == EN_PRESET && btn & 1u << B_SEL)
            pattern_pick(d);             /* SEL held: the pattern, on any page */
        else if (k == EN_PRESET && ui.page == PG_FX)
            fx_type_turn(d);             /* the focused slot's effect */
        else if (k == EN_PRESET && ui.page == PG_PERC)   /* LOAF BREAKER: the snare's, hat's or clap's memories */
            perc_mem_turn(ui.pvoice, d);
        else if (k == EN_PRESET && ui.page == PG_KICK) {   /* LOAF BREAKER: the kick memories; on KNOBS' K1 SET ..
                                          * K4 SET: that knob's function (as ENV KNOB's PRESETS) */
            if (ui.kpage == KP_KSET)
                selk_assign_k(ui.kks, d);
            else if (ui.kpage == KP_STYLE)
                kstyle_turn(d);          /* (KNOBS' STYLE: a style, the four knobs at once) */
            else
                kick_mem_turn(d);
        } else if (k == EN_PRESET && ui.page == PG_TRACK && ui.tpage == 2u) {   /* LOAF BREAKER: DROP's BUILD KICK */
            char t[12];
            set.drop_kick = (uint8_t)clamp((int32_t)set.drop_kick + (d > 0 ? 1 : -1), 0, (int32_t)KICK_MEMS);
            t[0] = 0;
            drop_kick_text(t);
            set_dirty = 1u;
            app_say("BUILD KICK", t);
            ui.redraw |= RG_BLOCKS;
        } else if (k == EN_PRESET && ui.page == PG_TRACK && ui.tpage < 2u)
            track_pick(d);               /* LOAF BREAKER: ENV: the track */
        else if (k == EN_PRESET && ui.page == PG_TRACK && ui.tpage == 4u) {   /* LOAF BREAKER TEST: BUILD's TEMPO RISE */
            char t[8];
            set.drop_accel = (uint8_t)clamp((int32_t)set.drop_accel + (d > 0 ? 2 : -2), 0, 30);
            changed();
            t[0] = 0;
            if (!set.drop_accel)
                text_cat(t, "OFF");
            else {
                t[0] = '+';
                int_text(set.drop_accel, t + 1);
                text_cat(t, "%");
            }
            app_say("TEMPO RISE", t);
            ui.redraw |= RG_BLOCKS;
        }
        else if (k == EN_PRESET && ui.page == PG_GLOBAL && ui.gpage == 2u) {   /* LOAF BREAKER: GLO MORE: WAVE ZOOM,
                                          * how much of the past the scrolling wave holds */
            set.wzoom = (uint8_t)clamp((int32_t)(set.wzoom % CDJ_ZN) + (d > 0 ? 1 : -1), 0, CDJ_ZN - 1);
            changed();
            app_say("WAVE ZOOM", CDJ_ZNAME[set.wzoom % CDJ_ZN]);
            ui.redraw |= RG_BLOCKS;
        } else if (k == EN_PRESET)
            pattern_pick(d);             /* elsewhere: the pattern */
        else if (ui.kil_focus && k == EN_K1) {   /* LOAF BREAKER: KIL held, K1: its LATCH, right on (held now: it
                                          * stays quiet), left off */
            set.half_mt_off = (uint8_t)(d > 0 ? set.half_mt_off | 4u : set.half_mt_off & ~4u);
            if (d > 0)
                kick_hold = 1u;
            set_dirty = 1u;
            app_say("KIL LATCH", d > 0 ? "ON" : "OFF");
            ui.redraw |= RG_FOOT;
        } else if (key_tuned() >= 0)     /* a white key held: K1-K4 tune its loop */
            key_knob_turn(key_tuned(), k - EN_K1, d);
        else if (ui.pad_focus >= 0)      /* a pad held: K1-K4 tune it */
            pad_knob_turn(ui.pad_focus, k - EN_K1, d);
        else
            knob_turn(k, d);
    }
    ui.keys = keys;
    ui.btn = btn;
    apply_flush();                       /* (TEST: the pass's changes, applied once) */
}

void app_input_only(void) { input(); }

static void leds(void)
{
    static uint8_t after_intro;
    brk_status_t st;
    uint32_t b = 0u, k = 0u, i;
    if (demo.on) {                       /* the demo's own (demo.c) */
        demo_leds(plat_ms());
        after_intro = 1u;
        return;
    }
    if (intro_leds(plat_ms())) {         /* the start screen's own (screen.c) */
        after_intro = 1u;
        return;
    }
    if (after_intro) {                   /* (its mid-lit tail let go) */
        after_intro = 0u;
        plat_lights_mid(0u, 0u);
    }
    brk_status(&st);
    if (ui.sel_lock)                     /* (LOAF BREAKER: SEL locked: lit) */
        b |= 1u << B_SEL;
    if (st.playing && plat_ms() - ui.kick_t >= 70u)   /* (LOAF BREAKER TEST: PLAY blinks off on each kick) */
        b |= 1u << B_PLAY;
    static const uint8_t PAGE_BTN[NPAGES] = {B_HOME, B_HOME, B_FX, B_GLO, B_EDIT, B_LFO, B_SEQ, B_EDIT, B_ENV, B_ARP,
                                             B_EDIT};   /* (TEST: KICK EQ under EDIT) */
    b |= 1u << (IS_KFX() || (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK) ? B_EDIT : ui.page == PG_GLOBAL &&
                ui.gpage == GP_FILL ? B_SEQ : PAGE_BTN[ui.page]);   /* (LOAF BREAKER: SOUNDS' BREAK, PATTERN's GROOVE) */
    if (ui.rec)
        b |= 1u << B_REC;
    if (set_dirty)
        b |= 1u << B_SAVE;
    if (!slices_page()) {                /* a lane's page: its steps lit, the step playing flipped */
        uint32_t m = keys_lit(&st);
        for (i = 0; i < 16u; i++)
            if (m >> i & 1u)
                k |= 1u << WK[i];
    } else if (st.slice >= 0 && st.slice < 16)
        k |= 1u << WK[st.slice];
    for (i = 0; i < 11u; i++)
        if (i == PAD_FREEZE ? sub_drive_on() : i == PAD_DUB ? ovl_pad_on() : (int)(st.pads >> i & 1u) || roll_lit(i))
            k |= PAD_BIT(i);
    if (kick_hold)                       /* (LOAF BREAKER: KIL held: lit) */
        k |= 1u << BK[KILL_KEY];
    if (KICK_TERROR(&set.kick) && !terror_off)   /* (LOAF BREAKER: TER lit while the kick's TERROR sounds) */
        k |= 1u << BK[TERR_KEY];
    if (build_p >= 0.0f) {               /* LOAF BREAKER: the build: the white keys fill up left to right (TEST: the next
                                          * one blinking, faster as it builds) */
        uint32_t n = 1u + (uint32_t)(build_p * 15.0f), per = 220u - (uint32_t)(build_p * 180.0f);
        for (i = 0; i < 16u; i++)
            k = i < n ? k | 1u << WK[i] : k & ~(1u << WK[i]);
        if (n < 16u && (plat_ms() / per) & 1u)
            k |= 1u << WK[n];
    } else if (ui.drop_t) {
        if ((plat_ms() / 60u) & 1u) {    /* the drop: every light strobing */
            b = 0x3FFFu;
            k = 0x7FFFFFFu;
        } else
            b = k = 0u;
    }
    plat_glow(1);
    plat_leds(b, k);
}

/* ------------------------------------------------------------ slicer view --- */
static void knob_value(int i, char *t)  /* the page's knob i (0..3), as shown under its name */
{
    t[0] = 0;
    if (ui.page == PG_TRACK || RST_ON) {
        track_value(i, t);
        return;
    }
    if (ui.page == PG_GLOBAL) {
        global_value(i, t);
        return;
    }
    if (ui.page == PG_KICK) {
        kick_value(i, t);
        return;
    }
    if (ui.page == PG_SEQ) {
        seq_value(i, t);
        return;
    }
    if (ui.page == PG_PERC) {
        perc_value(i, t);
        return;
    }
    if (ui.page == PG_MIX) {
        mix_value(ui.mpage, i, t);
        return;
    }
    if (ui.page == PG_HOME) {
        if (i == 0) {
            if (!set.gate || set.gate >= 100u)
                text_cat(t, "OFF");
            else {
                int_text(set.gate, t);
                text_cat(t, "%");
            }
        } else if (i == 2)               /* GLOUPI BREAKS: the kick's filter (SEQ's K2 the same) */
            kick_flt_text(t);
        else if (i == 1)                 /* the break's DJ filter (LOAF BREAKER: a low-pass) */
            brk_flt_text(t);
        else {                         /* GLOUPI BREAKS: the kick's SUB decay */
            int_text((int32_t)(kick_decay_ms(KL_SUB, set.kick.l[KL_SUB].decay) + 0.5f), t);
            text_cat(t, " MS");
        }
    } else if (ui.page == PG_KEQ)
        keq_value(i, t);
    else if (ui.page == PG_GEN) {
        float v = i == 0 ? set.gen.complexity : i == 1 ? set.gen.anchor : i == 2 ? set.gen.roll : set.gen.fill;
        int_text((int32_t)(v * 100.0f), t);
        text_cat(t, "%");
    } else
        fx_value(ui.fx_focus, i, t);
}

/* the tab bar under the header: the button's name, then the page's parts as tabs (the one you are on lit: the
 * button again, or ALGORITHM, goes to the next); EDIT and FX also name the part edited, "< SUB FX >" */
static int32_t tab(int32_t x, const char *s, int on)
{
    int32_t w = text_w(&FONT_T, s) + 8;
    cv_rect(x, 0, w, 13, on ? C_HI : C_BLOCK);
    cv_text(x + 4, 1, &FONT_T, s, on ? C_BLACK : C_GRAY);
    return x + w + 3;
}
/* the pages' buttons as printed, GLOBAL's parts, SEQ's lanes: for the tabs and the header (page_say) */
static const char *const BTN[NPAGES] = {"HOME", "HOME", "FX", "GLO", "EDIT", "LFO", "SEQ", "EDIT", "ENV", "ARP",
                                        "EDIT"};
static const char *const GPART[NGPART] = {"MIDI", "PITCH", "SETUP", "EFFECTS", "FX SET", "RESET"};
/* LOAF BREAKER (0.201): each button a domain, its name on the screen (the button keeps its print): LIVE (HOME), SOUNDS
 * (EDIT: the kick, the drums, the melody, the break), PATTERN (SEQ), SONG (ENV), MIX (LFO), EFFECTS (FX), SYSTEM (GLO) */
static const char *const DOM[NPAGES] = {"LIVE", "LIVE", "EFFECTS", "SYSTEM", "SOUNDS", "MIX", "PATTERN", "SOUNDS", "SONG",
                                        "OVERLAY", "SOUNDS"};
static const char *dom_name(void)
{
    if (IS_KFX() || (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK))
        return "SOUNDS";
    if (ui.page == PG_GLOBAL && ui.gpage == GP_FILL)
        return "PATTERN";
    return DOM[ui.page % NPAGES];
}
static const char *const SEQL[NSEQL] = {"1-16", "17-32", "K1", "K2", "K3", "K4", "S1", "S2", "H1", "H2", "C1", "C2",
                                         "V1", "V2"};   /* (LOAF BREAKER: the HOOVER's, in the stab's place) */
static void draw_tabs(void)
{
    uint32_t i;
    int32_t x;
    char part[20];
    part[0] = 0;
    cv_begin(240, 14, C_BLACK);
    {                                    /* (LOAF BREAKER: the domain; SOUNDS: the source, KICK, DRUMS, MELODY, BREAK) */
        const char *lab = ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() ? "KICK" : ui.page == PG_PERC
                          ? (ui.einst == DR_STAB ? "MELODY" : "DRUMS") : ui.page == PG_GLOBAL && ui.gpage == GP_BREAK
                          ? "BREAK" : dom_name();
        x = ui.page == PG_TRACK || ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() ? 2   /* (LOAF BREAKER: the
                                          * KICK menu's six sections want the room; its picture says KICK n) */
            : cv_text(4, 1, &FONT_T, lab, C_WHITE) + 6;
    }
    if (ui.page == PG_HOME || ui.page == PG_GEN) {   /* HOME: PERFORM, GENERATOR (TEST: the kick's EQ and FX on EDIT) */
        x = tab(x, "PERFORM", ui.page == PG_HOME);
        tab(x, "GENERATOR", ui.page == PG_GEN);
    }
    else if (ui.page == PG_TRACK)        /* LOAF BREAKER: ENV: TRACKS, PLAY, DROP */
        for (i = 0; i < NTORD; i++)   /* (TRACKS, PLAY, DROP, BUILD: no "ENV" before them, for the room) */
            x = tab(x, TPART[TORDER[i]], TORDER[i] == ui.tpage);
    else if (ui.page == PG_GLOBAL && ui.gpage == GP_FILL) {   /* (LOAF BREAKER: PATTERN's GROOVE) */
        char pn[6] = "P";
        int_text((int32_t)pat_cur() + 1, pn + 1);
        x = tab(x, pn, 0);
        tab(x, "GROOVE", 1);
    } else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK)   /* (SOUNDS' BREAK: the break playing) */
        tab(x, break_name(set.brk), 1);
    else if (ui.page == PG_GLOBAL)
        for (i = 0; i < NGLO; i++)
            x = tab(x, GPART[GLO_ORD[i]], GLO_ORD[i] == ui.gpage);
    else if (ui.page == PG_MIX) {
        x = tab(x, "LEVELS", !ui.mpage);
        x = tab(x, "SENDS", ui.mpage == 1u);
        x = tab(x, "MASTER", ui.mpage == 2u);
        tab(x, "DYN", ui.mpage == 3u);   /* (LOAF BREAKER: COMP's and DUCK's timing, what DUCK ducks) */
    } else if (ui.page == PG_SEQ) {
        char pn[6] = "P";                /* LOAF BREAKER: the pattern, then the lane the keys edit (SEQ again, or
                                          * ALGORITHM: the next); the lanes themselves on the page */
        int_text((int32_t)pat_cur() + 1, pn + 1);
        if (pat_next >= 0)
            text_cat(pn, ">");
        x = tab(x, pn, 0);
        if (ui.piano) {                  /* (LOAF BREAKER: KEYS, the pattern's instrument: "ACID KEYS") */
            char kt[12] = "";
            text_cat(kt, MEL_NAME[mel_inst()]);
            text_cat(kt, " KEYS");
            tab(x, kt, 1);
        } else
            tab(x, SEQ_LANE_NAME[ui.seq_lane % NSEQL], 1);
    } else if (ui.page == PG_FX && !IS_KFX()) {
        for (i = 0; i < BRK_FX_SLOTS; i++)
            x = tab(x, SLOT_NUM[i], i == fx_foc());
        text_cat(part, fx_name(fx_foc()));
        if (ui.fx_page) {                /* "< MB COMP X-OVER >", "< FILTER MOD >" */
            text_cat(part, " ");
            text_cat(part, fx_part_name());
        }
    } else if (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX()) {   /* LOAF BREAKER: the KICK menu: its sections,
                                          * the part (if it fits) */
        for (i = 0; i < KSECTS; i++)
            x = tab(x, KSECT_NAME[i], i == ui.ksect % KSECTS);
        kick_part_name(kick_part(), part);
        if (x + text_w(&FONT_T, part) + 16 > 236)
            part[0] = 0;
    } else {                             /* SOUNDS: the drum, and its part (LOAF BREAKER: the melody its instrument) */
        if (ui.einst == DR_STAB)
            x = tab(x, MEL_NAME[mel_inst()], 1);
        else
            for (i = 1; i < DR_STAB; i++)
                x = tab(x, DRUM_TAG[i], i == ui.einst);   /* (SN HH CP) */
        text_cat(part, ui.ppage ? "SHAPE" : "SOUND");
    }
    if (part[0]) {                       /* "< PART >": ALGORITHM turns it */
        char t[24] = "< ";
        text_cat(t, part);
        text_cat(t, " >");
        cv_text(236 - text_w(&FONT_T, t), 1, &FONT_T, t, C_HI);
    }
    cv_blit(0, 30);                      /* (right under the header, on every page) */
}

/* the header, when a page opens or turns to another part: the button as printed and the part as its tab shows it
 * ("GLO PITCH", "EDIT KICK SUB FX", "FX SLOT 2 TREMOLO", "SEQ HAT", "ENV TAP TEMPO") */
static void page_say(void)
{
    char t[24];
    t[0] = 0;
    switch (ui.page) {
    case PG_HOME: text_cat(t, "PERFORM"); break;
    case PG_GEN: text_cat(t, "GENERATOR"); break;
    case PG_KEQ: text_cat(t, ui.keq_part ? "KICK EQ SHAPE" : "KICK EQ BANDS"); break;
    case PG_GLOBAL: text_cat(t, ui.gpage == GP_FILL ? "GROOVE" : ui.gpage == GP_BREAK ? "BREAK" : GPART[ui.gpage % NGPART]); break;
    case PG_MIX: text_cat(t, ui.mpage == 3u ? "DYNAMICS" : ui.mpage == 2u ? "MASTER" : ui.mpage ? "SENDS" : "LEVELS"); break;
    case PG_SEQ: text_cat(t, ui.piano ? "KEYS" : SEQL[ui.seq_lane % NSEQL]); break;
    case PG_TRACK: text_cat(t, TPART[ui.tpage % NTPART]); break;
    case PG_OVL: text_cat(t, "OVERLAY"); break;
    case PG_FX:
        text_cat(t, IS_KFX() ? "KICK FX " : "SLOT ");
        int_text((int32_t)(fx_foc() % BRK_FX_SLOTS) + 1, t + (IS_KFX() ? 8 : 5));
        text_cat(t, " ");
        text_cat(t, fx_name(fx_foc()));
        if (ui.fx_page) {
            text_cat(t, " ");
            text_cat(t, fx_part_name());
        }
        break;
    case PG_KICK:
        text_cat(t, "KICK ");
        text_cat(t, KSECT_NAME[ui.ksect % KSECTS]);
        text_cat(t, " ");
        kick_part_name(kick_part(), t);
        break;
    default:
        text_cat(t, PERC_VOICE_NAME[ui.pvoice % PERC_VOICES]);
        text_cat(t, ui.ppage ? " SHAPE" : " SOUND");
        break;
    }
    app_say(dom_name(), t);
}

/* SEL held alone: its combinations, on the strip (FX page: its own two) */
static void draw_help(void)
{
    /* LOAF BREAKER TEST: above, SEL's combinations on any page; below, this page's own ("+": with SEL; on HOME and the
     * pages with no shortcuts of their own: SEL + K1..K4, the functions EDIT KICK's KNOBS gave them) */
    static const char *const G[8][2] = {{"+EDIT", "NEXT SOURCE"}, {"+SEQ", "COPY / PASTE"}, {"+ENV", "BUILD / DROP"},
                                        {"+ARP", "GROOVE"}, {"+REC", "CLEAR"}, {"+SAVE", "UNDO"}, {"+ALGO", "BREAK"},
                                        {"+SELECT", "DRUM FILTER"}};   /* (LOAF BREAKER 0.201: SEL + a button, one
                                          * table: its domain's second function) */
    static const char *const P[10][4][2] = {
        {{"EDIT HELD", "BIG / CLASSIC VIEW"}, {"EDIT", "NEXT SECTION"}, {"PRESETS", "KICK 1-10"},
         {"SELECT", "NEXT DRUM (BIG VIEW)"}},
        {{"EDIT HELD", "BIG / CLASSIC VIEW"}, {"EDIT", "NEXT DRUM / PART"}, {"PRESETS", "MEMORY / INSTRUMENT"},
         {"SELECT", "NEXT DRUM (BIG VIEW)"}},
        {{"SEQ + KEY 1-4", "COPY THE BAR THERE"}, {"SEQ + K1", "SHIFT THE LANE"}, {"SEQ + REC", "CLEAR THE DRUMS"},
         {"+ SAVE", "UNDO"}},
        {{"+ FX", "BYPASS SLOT"}, {"FX + ALGO", "MOVE SLOT"}, {"+ REC", "RESET EFFECT"}, {"PRESETS", "EFFECT TYPE"}},
        {{"+ PLAY", "START ITEM (TRACKS)"}, {"PLAY", "PLAY THE TRACK"}, {"PRESETS", "THE TRACK"},
         {"WHITE KEYS", "PATTERNS / TRACKS"}},
        {{"LFO + K1", "BREAK LEVEL"}, {"LFO + K2", "KICK LEVEL"}, {"LFO + K3", "DRUMS LEVEL"}, {"LFO + K4", "MELODY LEVEL"}},
        {{"SETUP: K1", "BEGINNER / EXPERT"}, {"RESET", "ONE PART AT A TIME"}, {"RESET: ALL", "EVERYTHING, 15 TURNS"},
         {"SEL x2", "SEL LOCKED, LIT"}},
        {{"+ K1", ""}, {"+ K2", ""}, {"+ K3", ""}, {"+ K4", ""}},
        {{"ALGO", "THE BREAK"}, {"K1 - K3", "PITCH, GRAIN, SNAP"}, {"K4", "ITS LEVEL"}, {"+ EDIT", "THE KICK"}},
        {{"ARP", "FILL (LATCH: ON / OFF)"}, {"ALGO", "SEQ'S LANES"}, {"K1", "SWING"}, {"K2 - K4", "THE FILL"}}};
    uint32_t i, w = ui.page == PG_KICK ? 0u : ui.page == PG_PERC ? 1u : ui.page == PG_SEQ ? 2u : ui.page == PG_FX ? 3u
                    : ui.page == PG_TRACK ? 4u : ui.page == PG_MIX ? 5u : ui.page == PG_GLOBAL && ui.gpage == GP_BREAK
                    ? 8u : ui.page == PG_GLOBAL && ui.gpage == GP_FILL ? 9u : ui.page == PG_GLOBAL ? 6u : 7u;
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < 4u; i++) {           /* (SEL + a button: two columns) */
        cv_text(6, (int32_t)i * 13, &FONT_XS, G[i][0], C_HI);
        cv_text(46, (int32_t)i * 13, &FONT_XS, G[i][1], C_WHITE);
        cv_text(124, (int32_t)i * 13, &FONT_XS, G[i + 4u][0], C_HI);
        cv_text(170, (int32_t)i * 13, &FONT_XS, G[i + 4u][1], C_WHITE);
    }
    cv_blit(0, 88u);
    cv_begin(240, 52, C_BLACK);
    cv_rect(10, 0, 220, 1, C_LINE);      /* (this page's, under a line) */
    for (i = 0; i < 4u; i++) {
        const char *b = P[w][i][1];
        if (w == 7u)                     /* (the knobs: what KNOBS gave them) */
            b = SK_NAME[selk_fn(i)];
        if (w == 0u && i == 3u && ui.kpage == KP_FX) {
            cv_text(10, 3 + (int32_t)i * 12, &FONT_XS, "+ REC", C_HI);
            cv_text(104, 3 + (int32_t)i * 12, &FONT_XS, "RESET EFFECT", C_WHITE);
            continue;
        }
        cv_text(10, 3 + (int32_t)i * 12, &FONT_XS, P[w][i][0], C_HI);
        cv_text(104, 3 + (int32_t)i * 12, &FONT_XS, b, C_WHITE);
    }
    cv_blit(0, 140u);
}

/* LOAF BREAKER TEST: a knob's value as an arc (a 270 degree gauge, 13 px, lit as far as it is turned), where the page
 * knows it 0..1: the FX slots, the kick, the MIXER's levels */
static const int8_t ARC[19][2] = {{-4, 4}, {-5, 3}, {-6, 2}, {-6, 0}, {-6, -2}, {-5, -3}, {-4, -4}, {-3, -5}, {-2, -6},
                                  {0, -6}, {2, -6}, {3, -5}, {4, -4}, {5, -3}, {6, -2}, {6, 0}, {6, 2}, {5, 3}, {4, 4}};
static void knob_arc(int32_t cx, int32_t cy, float v)
{
    int32_t j, lit = (int32_t)(brk_clampf(v, 0.0f, 1.0f) * 18.0f + 0.5f);
    for (j = 0; j < 19; j++)
        cv_rect(cx + ARC[j][0], cy + ARC[j][1], j <= lit ? 2 : 1, j <= lit ? 2 : 1, j <= lit ? C_HI : C_LINE);
}
static float foot_norm(int i)            /* the page's knob i, 0..1, or -1 (no gauge) */
{
    if (ui.page == PG_FX && !mb_detail(fx_foc()) && !x_detail(fx_foc())) {
        const brk_fxslot_t *sl = fx_sl(fx_foc());
        return sl->type ? (i == 3 ? sl->mix : sl->p[i & 3]) : -1.0f;
    }
    if (ui.page == PG_KICK && !ui.kclassic)
        return -1.0f;                    /* (the big view: its cards have their bars) */
    if (ui.page == PG_KICK)
        return kick_knob_f(i);
    if (ui.page == PG_MIX && !ui.mpage)
        return i == 0 ? (float)set.brk_level * 0.01f : i == 1 ? set.kick.volume : i == 2 ? set.perc.volume
               : (float)set.mel_vol * 0.01f;
    return -1.0f;
}
/* the foot: the pads, then the page and its knobs with their values */
static void draw_foot(const brk_status_t *st)
{
    static const char *const KN[NPAGES][4] = {{"GATE", "FILTER", "DRUM FLT", "SUB DECAY"},
                                              {"CMPLX", "ANCHOR", "ROLL", "FILL"},
                                              {"P1", "P2", "P3", "MIX"},
                                              {"CLOCK IN", "CLOCK OUT", "", ""},
                                              {"", "", "", ""},
                                              {"BREAK", "KICK", "PERC", "MELO"},
                                              {"SLICE", "PITCH", "HITS", "REVERSE"},
                                              {"", "", "", ""},
                                              {"KICK", "PUT", "LOOPS", "BREAK"},
                                              {"AMOUNT", "RATE", "SWEEP", "LATCH"},
                                              {"LOW", "MID", "HIGH", "FREQ"}};
    uint32_t i;
    char t[16];
    draw_tabs();
    cv_begin(240, 48, C_BLACK);
    for (i = 0; i < 11u; i++) {
        int on = i == PAD_FREEZE ? sub_drive_on() : i == PAD_DUB ? ovl_pad_on() : (int)((st->pads >> i) & 1u) || roll_lit(i);
        int32_t x = 2 + (int32_t)PAD_KEY[i] * 21;   /* (at its key) */
        if (PAD_KEY[i] >= 11u)
            continue;                    /* (LOAF BREAKER: GATE, no key) */
        cv_rect(x, 0, 20, 14, on ? C_HI : C_BLOCK);
        cv_text(x + (20 - text_w(&FONT_T, PAD_NAME[i])) / 2, 1, &FONT_T, PAD_NAME[i], on ? C_BLACK : C_GRAY);
    }
    {                                    /* LOAF BREAKER: KIL, the sequencer's kicks held quiet */
        int32_t x = 2 + (int32_t)KILL_KEY * 21;
        cv_rect(x, 0, 20, 14, kick_hold ? C_HI : C_BLOCK);
        cv_text(x + (20 - text_w(&FONT_T, "KIL")) / 2, 1, &FONT_T, "KIL", kick_hold ? C_BLACK : C_GRAY);
    }
    {                                    /* LOAF BREAKER: TER, lit while the kick's TERROR sounds */
        int32_t x = 2 + (int32_t)TERR_KEY * 21;
        int on = KICK_TERROR(&set.kick) && !terror_off;
        cv_rect(x, 0, 20, 14, on ? C_HI : C_BLOCK);
        cv_text(x + (20 - text_w(&FONT_T, "DST")) / 2, 1, &FONT_T, "DST", on ? C_BLACK : C_GRAY);
    }
    if (ui.page == PG_SEQ && ui.piano)   /* LOAF BREAKER: HV KEYS: the black keys are notes (their names) */
        for (i = 0; i < 11u; i++) {
            int32_t x = 2 + (int32_t)i * 21, n = (int32_t)PIANO_B[i] - 12 + 12 * ui.poct;
            int on = ui.pnote_t && plat_ms() - ui.pnote_t < 600u && ui.pnote == n;
            char nn[6];
            note_name(n, nn);
            nn[2] = 0;                   /* (the name alone: C#, D#..) */
            cv_rect(x, 0, 20, 14, on ? C_HI : C_BLOCK);
            cv_text(x + (20 - text_w(&FONT_T, nn)) / 2, 1, &FONT_T, nn, on ? C_BLACK : C_HI);
        }
    for (i = 0; i < 4u; i++) {
        static const char *const KN_PITCH[4] = {"PITCH", "GRAIN", "SNAP", "LEVEL"};   /* (SOUNDS' BREAK) */
        static const char *const KN_GROOVE[3][4] = {{"FX", "DRUMS", "", ""}, {"COMP", "RATIO", "DUCK", "MAX OUT"},
                                                    {"C ATTACK", "C RELEASE", "D RELEASE", "DUCKS"}};
        static const char *const KN_MORE[3][4] = {{"MODE", "AUTO SAVE", "METER", "SKIN"},   /* GLOBAL's MORE */
                                                  {"EFFECT", "ON / OFF", "", ""},   /* (LOAF BREAKER: EFFECTS, */
                                                  {"AMOUNT", "KICK", "DROP", "LENGTH"}};   /* FX SET) */
        static const char *const KN_TRK[4][4] = {{"TRACK", "FROM", "END", ""}, {"BUILD", "AFTER", "BREAK", "CRASH"},
                                                 {"STYLE", "SWEEP", "RISER", "GAP"}, {"WHAT", "RESET", "RESET", "RESET"}};
        static const char *const KN_FILL[4] = {"SWING", "LATCH", "DRUM", "LENGTH"};   /* (LOAF BREAKER: GROOVE) */
        const char *name = ui.page == PG_GLOBAL && ui.gpage == GP_FILL ? KN_FILL[i] : RST_ON ? KN_TRK[3][i] : ui.page == PG_TRACK &&
                           ui.tpage ? KN_TRK[ui.tpage == 4u ? 2u : (ui.tpage - 1u) & 1u][i] : ui.page == PG_GLOBAL &&
                           ui.gpage >= 2u ? KN_MORE[(ui.gpage - 2u) % 3u][i] : ui.page == PG_GLOBAL && ui.gpage
                           ? KN_PITCH[i] : ui.page == PG_MIX && ui.mpage
                           ? KN_GROOVE[ui.mpage >= 3u ? 2u : ui.mpage == 2u][i] : KN[ui.page][i];
        if (ui.shown >= 100) {           /* a white key held: its loop, or its slice's effect */
            cv_text(4 + (int32_t)i * 60, 18, &FONT_T, ui.key_layer ? SLX_KNOB[i] : KEY_KNOB[i], C_HI);
            if (ui.key_layer)
                slx_knob_value(ui.shown - 100, (int)i, t);
            else
                key_knob_value(ui.shown - 100, (int)i, t);
            cv_text(4 + (int32_t)i * 60, 30, &FONT_XS, t, C_WHITE);
            continue;
        }
        if (ui.kil_focus && i == 0u && ui.shown < 100) {   /* LOAF BREAKER: KIL held: K1 its LATCH */
            cv_text(4, 18, &FONT_T, "LATCH", C_HI);
            cv_text(4, 30, &FONT_XS, set.half_mt_off & 4u ? "ON" : "OFF", C_WHITE);
            continue;
        }
        if (ui.shown >= 0) {             /* a pad held: its own settings */
            name = pad_knob_name(ui.shown, (int)i);
            if (!name)
                continue;
            cv_text(4 + (int32_t)i * 60, 18, &FONT_T, name, C_HI);
            pad_knob_value(ui.shown, (int)i, t);
            cv_text(4 + (int32_t)i * 60, 30, &FONT_XS, t, C_WHITE);
            continue;
        }
        if (ui.page == PG_FX)            /* the focused slot's effect: its settings, then MIX */
            name = fx_param_name(ui.fx_focus, (int)i);
        else if (ui.page == PG_KICK)
            name = kick_knob_name((int)i);
        else if (ui.page == PG_PERC)
            name = perc_kname(ui.pvoice, ui.ppage, (int)i);
        else if (ui.page == PG_KEQ)
            name = KEQ_NAME[ui.keq_part & 1u][i];
        else if (ui.page == PG_SEQ && ui.piano) {   /* (LOAF BREAKER: HV KEYS) */
            static const char *const KN_PIANO[4] = {"STEP", "OCTAVE", "NOTE", "OUT / REST"};
            name = KN_PIANO[i];
        } else if (ui.page == PG_SEQ && ui.seq_lane >= SL_KICK) {
            static const char *const KN_LANE[2][4] = {{"FILL", "SOUND", "CLR / 4X4", "LENGTH"},
                                                      {"NOTE", "HITS", "COND", "NUDGE"}};   /* (LOAF BREAKER: a
                                          * step held, its own; K2 what the lane plays: KICK, the drum's SOUND, MELODY) */
            name = KN_LANE[ui.note_key != 0u][i];
            if (!ui.note_key && i == 1)
                name = sl_kick(ui.seq_lane) ? "KICK" : sl_drum(ui.seq_lane) == DR_STAB ? "MELODY" : "SOUND";
        }
        cv_text(4 + (int32_t)i * 60, 18, &FONT_T, name, ui.page == PG_FX
                ? (fx_sl(fx_foc())->on ? C_HI : C_DIM) : C_GRAY);
        knob_value((int)i, t);
        cv_text(4 + (int32_t)i * 60, 30, &FONT_XS, t, C_WHITE);
        {                                /* (TEST: its arc, where there is room beside the value) */
            float nv = foot_norm((int)i);
            if (nv >= 0.0f && text_w(&FONT_XS, t) <= 38)
                knob_arc(4 + (int32_t)i * 60 + 49, 37, nv);
        }
    }
    cv_blit(0, 192);
}

void app_ui_init(void)
{
    ab_take();                           /* (LOAF BREAKER: TOOLS' A/B: the kick as it was saved) */
    ui.ksect = 1u;                       /* (LOAF BREAKER, 0.205: the KICK menu opens on BODY, its MIX) */
#if CH_LOGO
    if (splash_t <= 1u)                  /* LOAF BREAKER: the splash stays (no clock when it was drawn: from now) */
        splash_t = plat_ms() | 1u;
#else
    lcd_fill(0, 0, 240, 240, C_BLACK);
#endif
    ui.last_step = ui.last_slice = -2;
    ui.intro = 1u;                       /* the start screen first */
    ui.intro_t0 = plat_ms();
    ui.idle_t = ui.intro_t0;
    ui.intro_drawn = 0u;
    ui.touched = 0u;
    ui.pad_focus = ui.key_focus = -1;
    ui.shown = -1;
    ui.redraw = RG_HEAD;
}

void app_frame(void)
{
    brk_status_t st;
    const brk_break_t *b;
    uint32_t now = plat_ms(), hdr;
    input();
    {                                    /* LOAF BREAKER TEST: GLO MORE's METER: the audio's load (average and peak, % of
                                          * its block), the strips sent to the screen and the passes, a second's worth */
        static uint32_t m_t, m_frames, m_blits, m_sum, m_n, m_pk;
        uint32_t c = plat_cpu_pct();
        m_frames++;
        m_sum += c;
        m_n++;
        m_pk = c > m_pk ? c : m_pk;
        if (now - m_t >= 1000u) {
            if (ui.meter_on && m_n) {
                char a[24] = "CPU ", v[24] = "IMG ", n[8];
                int_text((int32_t)(m_sum / m_n), n);
                text_cat(a, n);
                text_cat(a, "% PK ");
                int_text((int32_t)m_pk, n);
                text_cat(a, n);
                text_cat(a, "%");
                int_text((int32_t)(ui_blits - m_blits), n);
                text_cat(v, n);
                text_cat(v, " FR ");
                int_text((int32_t)m_frames, n);
                text_cat(v, n);
                app_say(a, v);
            }
            m_t = now;
            m_frames = m_sum = m_n = m_pk = 0u;
            m_blits = ui_blits;
        }
    }
    break_poll();
    if (splash_t) {                      /* LOAF BREAKER: the splash 1.5 s, then the start screen from its start */
        if (now - splash_t < 1500u) {
            static uint32_t gl_drawn;    /* (the glitch, settling in the first 700 ms, a frame each 40 ms) */
            if (now - splash_t < 760u && now - gl_drawn >= 40u) {
                gl_drawn = now;
                splash_glitch(now - splash_t);
            }
            ui.touched = 0u;
            return;
        }
        splash_t = 0u;
        ui.intro_t0 = now;
        ui.intro_drawn = 0u;
        ui.redraw = RG_ALL;
    }
    if (ui.touched)                      /* (the time itself: "now | 1" was a millisecond ahead half the time, and
                                          * now - idle_t then wrapped round to the demo at once) */
        ui.idle_t = now;
    if (demo.on) {                       /* the demo: until anything is touched (that touch does nothing else) */
        if (ui.touched) {
            ui.touched = 0u;
            demo_end();
        } else {
            leds();
            ch_bta_poll(now);
            demo_frame(now);
            return;
        }
    }
#if CH_DEMO
    else if (!ui.intro && !ui.lobo_t &&
             (int32_t)(now - ui.idle_t) > (int32_t)(playing_now() ? DEMO_IDLE_PLAY_MS : DEMO_IDLE_MS)) {
        slices_let_go(ui.keys);
        ui.help = 0u;
        demo_start(now);
        leds();
        demo_frame(now);
        return;
    }
#endif
    leds();
    if (ui.intro) {                      /* any key or button opens the slicer (PLAY also starts the break) */
        if (ui.touched || now - ui.intro_t0 > 16u * INTRO_STEP + INTRO_HOLD)
            intro_end();
        else {
            intro_draw(now);
            return;
        }
    }
    if ((ui.page == PG_TRACK) != (ui.drawn_page == PG_TRACK)) {   /* into or out of ENV: the whole screen again */
        lcd_fill(0, 44, 240, 148, C_BLACK);
        ui.redraw = RG_ALL;
    }
    ui.drawn_page = ui.page;
    if (build_p >= 0.0f && trk_drop_brk() && !ui.drop_asked) {   /* LOAF BREAKER
                                          * (crossbreed): MORE's DROP BRK asked for in the bar that ends in the drop
                                          * (break_poll swaps it in on the drop's step) */
        brk_status_t bs;
        brk_status(&bs);
        if (bs.bar16 >= 0 && (ui.drop_early || build_p * (float)build_len + (float)(15 - bs.bar16) >= (float)build_len)) {
            ui.drop_asked = 1u;
            if (trk_drop_brk() - 1u != set.brk)
                break_ask(trk_drop_brk() - 1u);
        }
    }
    {                                    /* LOAF BREAKER TEST: BUILD's TEMPO RISE: the tempo up with the build, back at
                                          * the drop (not while following a clock) */
        static uint8_t ramped;
        if (build_p >= 0.0f && set.drop_accel && !ext_active()) {
            brk_tempo((float)set.tempo_x10 * 0.1f * (1.0f + (float)set.drop_accel * 0.01f * build_p));
            ramped = 1u;
        } else if (ramped) {
            ramped = 0u;
            if (!ext_active())
                brk_tempo((float)set.tempo_x10 * 0.1f);
        }
    }
    if (build_p >= 0.0f && now - ui.build_said >= 250u) {   /* LOAF BREAKER: the build, how far */
        char t[8];
        ui.build_said = now;
        int_text((int32_t)(build_p * 100.0f + 0.5f), t);
        text_cat(t, "%");
        app_say("BUILD", t);
    }
    {                                    /* LOAF BREAKER: a build begun: its own kick (DROP's PRESETS, a track's DROP
                                          * item's K1), the one before back at the drop (or stopped) */
        static int b_was;
        int b = build_p >= 0.0f;
        if (b && !b_was && drop_kick_want && drop_kick_want <= KICK_MEMS &&
            drop_kick_want - 1u != set.kslot % KICK_MEMS) {
            drop_kick_back = (uint8_t)(set.kslot % KICK_MEMS + 1u);
            kick_mem_go(drop_kick_want - 1u);
        }
        b_was = b;
        if (drop_kick_back && !st.playing) {
            kick_mem_go(drop_kick_back - 1u);
            drop_kick_back = drop_kick_want = 0u;
        }
    }
    if (drop_n != ui.drop_seen) {        /* LOAF BREAKER: a DROP: the screen's flash, 0.9 s; the next pattern (its
                                          * drums already playing: the render took them at the drop) */
        ui.drop_seen = drop_n;
        ui.drop_t = now | 1u;
        if (ui.drop_pat < NPAT_ALL) {    /* (LOAF BREAKER: from another group: in now, from its objects) */
            if (pat_go(ui.drop_pat)) {
                pat_next = -1;
                pat_apply();
                set_dirty = 1u;
            }
            ui.drop_pat = 0xFFu;
        }
        if (drop_kick_back) {            /* (LOAF BREAKER: the kick from before the build back; the pattern dropped
                                          * into's own, a track's, after it) */
            kick_mem_go(drop_kick_back - 1u);
            drop_kick_back = 0u;
        }
        drop_kick_want = 0u;
        track_dropped();                 /* (a track: on from there) */
    }
    if (ui.drop_t && now - ui.drop_t > 900u)   /* (LOAF BREAKER: the drop: the lights strobe 0.9 s, the screen as it
                                          * is: no flash screen) */
        ui.drop_t = 0u;
    if (ui.lobo_t) {                     /* RESET ALL's screen: 3 s, or until a key or button (after its own turn) */
        if (now - ui.lobo_t > 3000u || (ui.touched && now - ui.lobo_t > 500u)) {
            ui.lobo_t = 0u;
            lcd_fill(0, 0, 240, 240, C_BLACK);
            ui.redraw = RG_ALL;
        } else {
            ui.touched = 0u;
            return;
        }
    }
    ui.touched = 0u;
    ch_bta_poll(now);
    brk_status(&st);
    b = brk_get_break();
    {                                    /* LOAF BREAKER: AUTO SAVE. A change waits a minute, then is saved once the
                                          * break has been stopped and the output silent 10 s (a flash write stalls the
                                          * sound: never while playing, nor while anything still sounds) */
        static uint32_t dirty_t, stop_t; /* (stop_t: when the break last played or the output was last heard, over
                                          * -60 dB: a long kick tail, a reverb, an echo, a drum played by hand) */
        if (st.playing || !stop_t || out_sound)
            stop_t = now | 1u;
        out_sound = 0u;
        if (!set_dirty || !set.auto_save)
            dirty_t = 0u;
        else if (!dirty_t)
            dirty_t = now | 1u;
        else if (now - dirty_t >= 60000u && !st.playing && now - stop_t >= 10000u && !ui.lobo_t) {   /* (10 s: FLEXOR) */
            int ok = store_all();
            set_dirty = 0u;
            dirty_t = 0u;
            app_say("AUTO SAVE", ok ? "DONE" : "FAILED");
        }
    }
    {                                    /* LOAF BREAKER: the battery's level, looked at every 2 s (the header) */
        static uint32_t bat_t;
        static int bat_seen = -2;
        if (now - bat_t >= 2000u) {
            int bat = plat_battery();
            bat_t = now;
            if (bat != bat_seen) {
                bat_seen = bat;
                ui.redraw |= RG_HEAD;
            }
        }
    }
    track_poll(&st);                     /* LOAF BREAKER: a track playing: its next item */
    {                                    /* LOAF BREAKER: a STUTTER build: the break rolled, 8ths, 16ths, then 32nds,
                                          * let go at the drop */
        static int8_t stut = -1;
        int want = build_p >= 0.0f && build_len && b_style == DS_STUTTER
                   ? (build_p < 0.5f ? PAD_ROLL8 : build_p < 0.8f ? PAD_ROLL16 : PAD_ROLL32) : -1;
        if (want != stut) {
            if (stut >= 0)
                brk_pad(stut, 0);
            if (want >= 0)
                brk_pad(want, 1);
            stut = (int8_t)want;
        }
    }
    {                                    /* LOAF BREAKER: PERFORM and GENERATOR show the CDJ view (the default);
                                          * HOME held 0.35 s there: the slice blocks instead, held again: the CDJ
                                          * view (HOME's next part not taken); ~20 times a second (TEST) */
        int home = ui.page == PG_HOME || ui.page == PG_GEN;
        if (!(ui.btn & 1u << B_HOME))
            ui.cdj_skip = 0u;
        if (!ui.cdj_skip && home && ui.btn & 1u << B_HOME && now - ui.home_t >= 350u) {
            ui.blocks ^= 1u;
            ui.cdj_skip = 1u;
            ui.home_combo = 1u;
            ui.panel_drawn = 0u;
            ui.redraw |= RG_BLOCKS;
            app_say("HOME", ui.blocks ? "SLICES" : "WAVEFORM");
        }
        ui.cdj = (uint8_t)(home && !ui.blocks);
        if (ui.cdj && now - ui.panel_drawn >= 50u) {   /* (TEST: 20 times a second, was 30) */
            ui.redraw |= RG_BLOCKS;
            ui.panel_drawn = now;
        }
    }
    if (pat_next >= 0 && (!st.playing || (st.step >= 0 && (uint32_t)st.step + 1u >= pat_len()))) {
        int ok = pat_go((uint32_t)pat_next);   /* the pattern picked: in for the loop's next round (its last step
                                          * plays; LOAF BREAKER: another group's, read in now; refused: this one's
                                          * changes not saved, playing) */
        pat_next = -1;
        if (ok) {
            if (st.playing)
                brk_rewind();            /* LOAF BREAKER: from its step 1 and the drums' first bar, whatever the
                                          * lengths were */
            pat_apply();
            set_dirty = 1u;
        }
        ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
    }
    if (st.step != ui.last_step || st.slice != ui.last_slice) {   /* the playhead moved */
        ui.last_step = st.step;
        ui.last_slice = st.slice;
        ui.redraw |= RG_WAVE | RG_BLOCKS;
    }
    if (ui.keys != ui.last_keys) {
        ui.last_keys = ui.keys;
        ui.redraw |= RG_BLOCKS;
    }
    if (ui.page == PG_GLOBAL && now - ui.panel_drawn >= 200u)   /* the GLOBAL page's panel: five times a second */
        ui.redraw |= RG_BLOCKS;
    if (ui.page == PG_TRACK && ui.tpage == 1u) {   /* LOAF BREAKER: PLAY's radio: the needle glides, PRESS PLAY blinks */
        float want = (float)(set.trk % NTRACKS), d = want - ui.radio;
        if (d > 0.01f || d < -0.01f || now - ui.panel_drawn >= 100u) {
            ui.radio += d * 0.35f;
            if (d < 0.01f && d > -0.01f)
                ui.radio = want;
            ui.redraw |= RG_BLOCKS;
            ui.panel_drawn = now;
        }
    }
    if (ui.page == PG_MIX && ui.mpage == 2u &&
        set.comp && now - ui.panel_drawn >= 50u) {   /* (COMP's reduction moving) */
        ui.redraw |= RG_BLOCKS;
        ui.panel_drawn = now;
    }
    if (ui.page == PG_MIX && now - ui.mix_drawn >= 50u)   /* the MIXER's meters: about 20 times a second (TEST: was 30) */
        ui.redraw |= RG_BLOCKS;
    brk_fx_watch(ui.page == PG_FX ? fx_foc() : BRK_FX_ALL);   /* (the engine meters that slot alone) */
    {                                    /* the overload guard shed a slot: say so */
        static uint32_t shed_seen;
        uint32_t sh = brk_fx_shed();
        if (sh > shed_seen)
            app_say("OVERLOAD", "FX SLOT BYPASSED");
        shed_seen = sh;
    }
    if (ui.page == PG_FX && fx_sl(fx_foc())->type != FXT_EMPTY && now - ui.panel_drawn >= 66u)
        ui.redraw |= RG_BLOCKS;          /* an effect focused: its picture and meters, about 15 times a second */
    if (foot_dirty) {
        foot_dirty = 0u;
        ui.redraw |= RG_FOOT;
    }
    {                                    /* the strip: a held key's loop, a held pad's animation, or the blocks */
        int16_t want = -1;
        int kt = key_tuned();
        if (ui.help)                     /* SEL held: its combinations */
            want = -2;
        else if (kt >= 0 && (ui.key_turned || now - ui.key_since >= KEY_SHOW_MS))
            want = (int16_t)(100 + kt);
        else if (ui.pad_focus >= 0)
            want = ui.pad_focus;
        if (want != ui.shown) {
            if ((want >= 0) != (ui.shown >= 0) || want < -1 || ui.shown < -1)
                lcd_fill(0, 86, 240, 106, C_BLACK);
            ui.shown = want;
            ui.redraw |= RG_FOOT | (want != -1 ? RG_FX : RG_BLOCKS);
        }
    }
    if (ui.shown != -1) {                /* the animation runs at about 30 frames a second (the help card stays) */
        if (ui.shown == PAD_TAPESTOP)
            ui.reel += 0.06f * st.tape;
        if (ui.shown >= 0 && now - ui.fx_drawn >= 33u)
            ui.redraw |= RG_FX;
        ui.redraw &= ~(uint8_t)RG_BLOCKS;
    }
    if (st.pads != ui.last_pads) {
        ui.last_pads = st.pads;
        ui.redraw |= RG_FOOT;
    }
    if (ui.msg_until && now >= ui.msg_until) {
        ui.msg_until = 0u;
        ui.redraw |= RG_HEAD;
    }
    /* the header's own state: playing, the tempo, the break */
    if (!st.playing)
        ui.seq_manual = 0u;
    if (ui.page == PG_SEQ && st.playing && !ui.seq_manual && !ui.note_key && !ui.piano) {   /* LOAF BREAKER: a lane of 32 or 64
                                          * steps: the keys show the bar playing (ALGORITHM, SEQ again: not, until
                                          * stopped) */
        const pat_t *p = &pats.p[set.pattern % NPAT];
        uint32_t sl = ui.seq_lane % NSEQL, n = lane_len(p, sl) / 16u, bar, want;
        bar = sl < SL_KICK ? (st.step >= 16 ? 1u : 0u) : st.bar64 >= 0 ? (uint32_t)(st.bar64 >> 4) % n : 0u;
        want = sl - SL_BAR[sl] + bar;
        if (n > 1u && want != sl && want < NSEQL) {
            ui.seq_lane = (uint8_t)want;
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        }
    }
    if (kv_pend && ui.page == PG_KICK && !ui.kclassic && now - kv_t >= 100u)
        ui.redraw |= RG_BLOCKS;          /* (TEST: the kick's picture, its last change drawn) */
    if (ui.page == PG_KICK && !ui.kclassic && kscope_hit && now - ui.kscope_drawn >= 200u && st.playing) {
        ui.kscope_drawn = now;           /* (TEST: the KICK menu's live kick, redrawn as it sounds) */
        ui.redraw |= RG_BLOCKS;
    }
    if (ui.rst_n && now - ui.rst_arm >= 2000u) {   /* (LOAF BREAKER: RESET's clicks let go: counted from 0 again) */
        ui.rst_n = 0u;
        ui.redraw |= RG_BLOCKS | RG_FOOT;
    }
    {                                    /* (LOAF BREAKER: a screen effect's moment on a kick or the drop: the whole
                                          * screen redrawn as it starts and as it ends) */
        static int sfx_was;
        int l = set.sfx_on ? sfx_live(now) : 0;
        if (l != sfx_was) {
            sfx_was = l;
            ui.redraw = RG_ALL;
        }
    }
    if (ui.aud_t && now - ui.aud_t >= 160u) {   /* (LOAF BREAKER: a drum played on its key: its card lit, then not) */
        ui.aud_t = 0u;
        ui.redraw |= RG_BLOCKS;
    }
    if (kick_hit_n != ui.kick_seen) {    /* (TEST: the kick's flash) */
        ui.kick_seen = kick_hit_n;
        ui.kick_t = now;
    }
    hdr = (uint32_t)st.playing | (uint32_t)set.tempo_x10 << 1 | (uint32_t)set.brk << 13 | (b ? 1u : 0u) << 20 |
          (uint32_t)ext_active() << 21 | (uint32_t)set.pattern << 22 | (uint32_t)ui.rec << 26 |
          (uint32_t)(pat_next >= 0) << 27 | (uint32_t)(now - ui.kick_t < 90u) << 28 |
          (uint32_t)(st.bar16 >= 0 ? (st.bar16 >> 2) & 3 : 0) << 29 | (uint32_t)(st.bar16 >= 0 && !(st.bar16 & 3)) << 31;
    if (hdr != ui.last_hdr) {
        ui.last_hdr = hdr;
        ui.redraw |= RG_HEAD;
    }
    if (ui.redraw & RG_HEAD)
        draw_head(&st, b, now);
    if (ui.meter_x > 0 && now - ui.meter_drawn >= 33u) {   /* the level meter, about 30 times a second */
        draw_meter();
        ui.meter_drawn = now;
    }
    {                                    /* LOAF BREAKER: SEQ left, its grid's top goes: the waveform back */
        static uint8_t last_page;
        if (ui.page != last_page && (last_page == PG_SEQ || last_page == PG_KICK || last_page == PG_GLOBAL))
                                         /* (and the KICK menu's picture, GLOBAL's top) */
            ui.redraw |= RG_WAVE;
        last_page = ui.page;
    }
    {                                    /* LOAF BREAKER: the screen's rows known (rows_send) forgotten on a whole redraw
                                          * and every 2 s, whatever else drew on it then shown right again */
        static uint32_t forget_t;
        if ((ui.redraw & RG_ALL) == RG_ALL || now - forget_t >= 2000u) {
            rows_forget(0u, 240u);
            forget_t = now;
        }
    }
    if (!ui.redraw)
        return;
    if (ui.redraw & RG_WAVE && ui.page != PG_TRACK && ui.page != PG_SEQ && ui.page != PG_GLOBAL &&
        (ui.page != PG_KICK || ui.kclassic) &&
        (ui.page != PG_PERC || !ui.pbig))   /* (LOAF BREAKER: SEQ is a grid there) */
        draw_wave(&st, b);
    if (ui.redraw & RG_FX) {
        if (ui.shown >= 100)
            draw_key(ui.shown - 100, &st, now);
        else if (ui.shown >= 0)
            draw_fx(ui.shown, &st, now);
        else
            draw_help();
        ui.fx_drawn = now;
    }
    if (ui.redraw & RG_BLOCKS) {
        if (ui.page == PG_TRACK || RST_ON)
            draw_track();
        else if (ui.page == PG_SEQ)
            draw_seq(&st);
        else if (ui.page == PG_PERC)
            draw_perc(&st);
        else if (ui.page == PG_KEQ)
            draw_keq();
        else if (ui.page == PG_GLOBAL) {
            draw_global(&st);
            ui.panel_drawn = now;
        } else if (ui.page == PG_FX) {
            draw_chain();
            ui.panel_drawn = now;
        } else if (ui.page == PG_KICK)
            draw_kick(&st);
        else if (ui.page == PG_MIX) {
            draw_mixer();
            ui.mix_drawn = now;
        } else if (ui.cdj)
            draw_cdj(&st);
        else
            draw_blocks(&st, ui.keys);
    }
    if (ui.redraw & RG_FOOT)
        draw_foot(&st);
    ui.redraw = 0u;
}

#ifdef PLAT_HOST
/* the host simulator's commands: "splash" draws the power-on screen (the device draws it before app_init; the
 * simulator never does) */
static int host_fails;
int app_host_fails(void) { return host_fails; }
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    (void)a; (void)b; (void)c; (void)d;
    if (!strcmp(cmd, "ensure_play")) {   /* ensure_play: playing (tools/soak.py's end, whatever came before) */
        brk_status_t st;
        set.clk_in = 0u;                 /* (a random turn may have left it following a MIDI clock that never comes) */
        ext.i = ext.n = 0u;
        ext.run = 0u;
        brk_status(&st);
        if (!st.playing)
            transport(1, 0);
        return 1;
    }
    if (!strcmp(cmd, "splash")) {
        app_splash();
        return 1;
    }
    if (!strcmp(cmd, "kick_sub_decay")) {   /* kick_sub_decay MS: the SUB layer's decay to that (the knob's 2 % steps miss
                                          * some: tools' pictures) */
        float lo = 0.0f, hi = 1.0f, want = (float)atoi(a);
        int n;
        for (n = 0; n < 24; n++) {
            float m = (lo + hi) * 0.5f;
            if (kick_decay_ms(KL_SUB, m) < want)
                lo = m;
            else
                hi = m;
        }
        set.kick.l[KL_SUB].decay = lo;
        settings_apply();
        printf("  kick SUB decay %d ms\n", (int)(kick_decay_ms(KL_SUB, lo) + 0.5f));
        return 1;
    }
    if (!strcmp(cmd, "scope_dump")) {    /* scope_dump FILE: the CDJ view's ring (r g b peak a column), the newest's
                                          * index, the step and its phase now (tools: PRESS PLAY's picture) */
        FILE *f = fopen(a, "w");
        brk_status_t st;
        uint32_t i, w = scope_w;
        brk_status(&st);
        if (f) {
            fprintf(f, "%u %d %d\n", (unsigned)w, (int)st.bar16, (int)(st.phase * 1000.0f));
            for (i = 0; i < SCOPE_N; i++)
                fprintf(f, "%u %u %u %u\n", scope[i][0], scope[i][1], scope[i][2], scope[i][3]);
            fclose(f);
        }
        return 1;
    }
    if (!strcmp(cmd, "bta_sim")) {       /* bta_sim: a pretend Bluetooth audio (the host has no radio) for the page */
        bta_sim_on = 1u;
        return 1;
    }
    if (!strcmp(cmd, "expect_bpm")) {    /* expect_bpm LO HI (whole BPM): the tempo the engine plays at */
        int32_t x100 = (int32_t)(brk_get_tempo() * 100.0f + 0.5f), bad = x100 < atoi(a) * 100 || x100 > atoi(b) * 100;
        host_fails += bad ? 1 : 0;
        printf("%s tempo %d.%02d BPM (%s..%s)\n", bad ? "FAIL" : "  ok", (int)(x100 / 100), (int)(x100 % 100), a, b);
        return 1;
    }
    return 0;
}
/* the scenarios' checks of the UI's state (expect WHAT VALUE) */
int app_expect(const char *what, int *got)
{
    static const char *const W[] = {"page", "gpage", "kpage", "seq_lane", "fx_focus", "fx_on", "lane_kick",
                                    "lane_hat", "pattern", "brk", "brk_next", "help", "drums_chain", "lobo",
                                    "ovl_on", "ovl_last", "ovl_latch", "roll_pitch_x10", "demo", "fx_page",
                                    "mb_solo", "mb_thr_low", "mb_slot2_thr_low", "trk_on", "trk_pos", "fx_p1", "kfx_fa", "kfx", "knotes", "comp", "knfn", "half_on", "intensity", "kpage", "t_type", "t_screech", "drop_style", "selk1", "selk3_fn", "khits", "trk_cur", "trk_it", "fx_type", "mb_kick", "mb_pre", "fill", "kbass1", "accel", "trk_from", "arp_fx", "arp_amt", "arp_more", "arp_thru", "ovl_rate", "kpreset", "aud_d", "kslot", "dirty", "brk_lvl", "kick_vol", "sfx_on", "sel_lock", "kick_dj", "csub", "kmorph", "kmorph_to", "ktune", "pbig", "playing", "kbar1", "kbar2", "kbar3", "selk1_fn", "selk4_fn", "kdrop", "ter_off", "terror", "mpage", "pgrp", "grp_dirty", "piano", "pcur", "hvn1", "hvn2", "hvn3", "minst", "mel_vol", "einst", "kpre", "kl_mode", "kl_note", "kl_src", "kl_end", "ab", "kview", "ksect"};
    int v[96];
    uint32_t i;
    v[0] = ui.page;
    v[1] = ui.gpage;
    v[2] = ui.kpage;
    v[3] = ui.seq_lane;
    v[4] = ui.fx_focus;
    v[5] = fx_sl(fx_foc())->on;
    v[6] = pat_now()->lane[LANE_KICK];
    v[7] = pat_now()->lane[LANE_HAT];
    v[8] = (int)pat_cur();            /* (LOAF BREAKER: 0..63, the group's 16 after the others) */
    v[9] = set.brk;
    v[10] = brk_next;
    v[11] = ui.help;
    v[12] = set.drums_chain;
    v[13] = ui.lobo_t != 0u;
    v[14] = fill_on;
    v[15] = 0;
    v[16] = set.ovl_latch;
    v[17] = (int)(set.pads.roll_pitch * 10.0f + (set.pads.roll_pitch < 0.0f ? -0.5f : 0.5f));
    v[18] = demo.on;
    v[19] = ui.fx_page;
    v[20] = mbc_at(fx_foc())->solo;
    v[21] = (int)(brk_mb_thr_db(mbc_at(fx_foc())->thr[MB_LOW]) - 0.5f);   /* whole dB */
    v[22] = (int)(brk_mb_thr_db(set.mbc[1].thr[MB_LOW]) - 0.5f);
    v[23] = ui.trk_on;
    v[24] = ui.trk_pos;
    v[25] = (int)(fx_sl(fx_foc())->p[0] * 100.0f + 0.5f);
    v[26] = (int)(set.kick.l[ui.klayer % KICK_LAYERS].fa * 100.0f + 0.5f);
    v[27] = set.kick.l[ui.klayer % KICK_LAYERS].fx;
    for (i = 0, v[28] = 0; i < 64u; i++)  /* (the kick's steps with a note) */
        v[28] += kick_note(pat_now(), i) != 0;
    v[29] = set.comp;
    v[30] = ui.knfn;
    {
        brk_status_t hs;
        brk_status(&hs);
        v[31] = (int)(hs.pads >> PAD_HALF & 1u);
    }
    v[32] = (int)(set.kick.intensity * 100.0f + 0.5f);
    v[33] = ui.kpage;
    v[34] = KICK_T_TYPE(&set.kick);
    v[35] = KICK_T_SCREECH(&set.kick);
    v[36] = set.drop_style;
    v[37] = set.selk_asg2 & 15u;
    v[38] = (int)selk_fn(2u);
    {                                    /* (the kick's hits since the last look) */
        static uint32_t seen;
        v[39] = (int)(kick_hit_n - seen);
        if (!strcmp(what, "khits"))
            seen = kick_hit_n;
    }
    v[40] = ui.trk_cur;
    v[41] = trk_now()->it[ui.trk_cur % TRK_ITEMS];
    v[42] = (int)fx_sl(fx_foc())->type;
    v[43] = mb_kick_is(fx_foc());
    v[44] = (int)mb_preset(fx_foc());
    v[45] = fill_on;
    v[46] = set.kbass[1];
    v[47] = set.drop_accel;
    v[48] = TRK_FROM;
    v[49] = set.ovl_pad & 15u;
    v[50] = set.ovl_amt[set.ovl_pad & 15u];
    v[51] = ui.pad_more;
    v[52] = (set.ovl_pad >> 6) & 3u;
    v[53] = set.ovl_rate;
    v[54] = ui.kpreset;
    v[55] = ui.aud_t ? ui.aud_d : -1;
    v[56] = set.kslot;
    v[57] = set_dirty;
    v[58] = set.brk_level;
    v[59] = (int)(set.kick.volume * 100.0f + 0.5f);
    v[60] = set.sfx_on;
    v[61] = ui.sel_lock;
    v[62] = set.kick_dj;
    v[63] = set.csub;
    v[64] = set.kmorph;
    v[65] = set.kmorph_to;
    v[66] = set.ksyn[KS_TUNE];
    v[67] = ui.pbig;
    v[68] = playing_now();
    v[69] = (int)drum_bar(pat_now(), DR_KICK, 1u);
    v[70] = (int)drum_bar(pat_now(), DR_KICK, 2u);
    v[71] = (int)drum_bar(pat_now(), DR_KICK, 3u);
    v[72] = (int)selk_fn(0u);
    v[73] = (int)selk_fn(3u);
    v[74] = set.ksyn[KS_DROP];
    v[75] = terror_off ? 1 : 0;
    v[76] = KICK_TERROR(&set.kick);
    v[77] = ui.mpage;
    v[78] = set.pgrp;
    v[79] = grp_changed();
    v[80] = ui.piano;
    v[81] = ui.pcur;
    for (i = 0; i < 3u; i++)             /* (the hoover's steps 1-3: their notes, -99 none) */
        v[82 + i] = hv_on(i) ? (int)stab_note(pat_now(), i) : -99;
    v[85] = (int)mel_inst();
    v[86] = set.mel_vol;
    v[87] = ui.einst;
    v[88] = steps.kpre[0][8] + steps.kpre[1][8] + steps.kpre[2][8] + steps.kpre[3][8];
    v[89] = klab.cur.mode;
    v[90] = klab.cur.note;
    v[91] = klab.cur.src;
    v[92] = klab.cur.end;
    v[93] = ab_b;
    v[94] = ui.kclassic ? 2 : ui.kview;
    v[95] = ui.ksect;
    for (i = 0; i < 96u; i++)
        if (!strcmp(what, W[i])) {
            *got = v[i];
            return 1;
        }
    return 0;
}
#endif
