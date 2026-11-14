/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the controls (knobs, keys, buttons), the lights, the foot and the frame. (Part of app.c's unity:
 * included there in order.) */

/* LOAF BREAKER: KICK FX: the FX page on the kick's slots (4, 5); (TEST) EDIT KICK's FX 1, FX 2 parts */
#define IS_KFX() (ui.page == PG_FX && ui.fx_focus >= BRK_FX_SLOTS)
static void page_say(void);
static void home_back(void)              /* (LOAF BREAKER, 0.9.2, FLEXOR: a page's button again, where SELECT and
                                          * ALGORITHM move in it: back to HOME, the main page) */
{
    ui.page = PG_HOME;
    ui.redraw |= RG_FOOT | RG_BLOCKS;
    page_say();
}
/* LOAF BREAKER (0.9.26, FLEXOR): SEL + ARP in the KICK LAB and on the drums: copy, then on another one paste (as SEL +
 * SEQ the pattern). The kick: all of it from its memory (sound, SYNTH, knobs, FX section, KICK LAB), the volume kept;
 * a drum: its sound and extras, onto another memory of the same drum */
static int8_t clip_kick = -1;
static int8_t clip_pv = -1, clip_ps;
static perc_voice_t clip_pvoice;
static uint8_t clip_px[PX_N];
static void kick_clip(void)
{
    char t[16] = "KICK ";
    uint32_t cur = set.kslot % KICK_MEMS;
    if (clip_kick < 0 || (uint32_t)clip_kick == cur) {
        clip_kick = (int8_t)cur;
        int_text((int32_t)cur + 1, t + 5);
        app_say("COPIED", t);
        return;
    }
    kick_copy_from((uint32_t)clip_kick);
    int_text(clip_kick + 1, t + 5);
    text_cat(t, " > ");
    int_text((int32_t)cur + 1, t + text_n(t));
    app_say("PASTED", t);
    clip_kick = -1;
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void perc_clip(void)
{
    uint32_t v = ui.pvoice % PERC_VOICES, s = *perc_slot(v) % PERC_MEMS;
    char t[20];
    t[0] = 0;
    text_cat(t, PERC_VOICE_NAME[v]);
    text_cat(t, " ");
    if (clip_pv != (int8_t)v || (uint32_t)clip_ps == s) {   /* (nothing copied, the same, or another drum: copy) */
        clip_pv = (int8_t)v;
        clip_ps = (int8_t)s;
        clip_pvoice = *perc_voice(v);
        memcpy(clip_px, perc_x(v), PX_N);
        int_text((int32_t)s + 1, t + text_n(t));
        app_say("COPIED", t);
        return;
    }
    *perc_voice(v) = clip_pvoice;
    memcpy(perc_x(v), clip_px, PX_N);
    int_text(clip_ps + 1, t + text_n(t));
    text_cat(t, " > ");
    int_text((int32_t)s + 1, t + text_n(t));
    app_say("PASTED", t);
    clip_pv = -1;
    changed();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static int sel_bt_page(void)             /* (0.9.26, FLEXOR: the pages where SEL + ALGORITHM is BREAK TIME) */
{
    return ui.page <= PG_GEN || ui.page == PG_HSET || ui.page == PG_MIX || ui.page == PG_OVL ||
           (ui.page == PG_TRACK && ui.tpage) || (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK);
}
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
    if (ui.page == PG_OVL) {             /* (LOAF BREAKER, 0.244: the OVERLAY page back on ARP) */
        ovl_turn(k - EN_K1, d);
        return;
    }
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
    if (ui.page == PG_MIX) {             /* LEVELS: break, kick, the drums; SENDS: FX, the drums' route (LOAF BREAKER 0.201) */
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
            } else if (k == EN_K3)
                arp_thru_turn(d);        /* (0.9.29, FLEXOR: the ARP's effects on BREAK, KICK, THRU) */
            else
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
        }                                /* (K4: the MELODY's fader 0.200-0.208; the ARP's route 0.9.29-0.9.30: SENDS
                                          * alone since, FLEXOR) */
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
    } else if (ui.page == PG_HSET) {     /* (LOAF BREAKER, 0.9.2: HOME's SETTINGS) */
        hset_turn(k - EN_K1, d);
        return;
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
    kfx_all();                           /* (0.9.26: every kick's FX section as the defaults left it) */
    kx_all();                            /* (every kick's knobs as they start) */
    if (n_bank)
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
    app_say("REC", ui.rec ? (playing_now() ? "ON" : "ON  STEP 1") : "OFF");
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
            ui.tap_pend = 0u;            /* (LOAF BREAKER, 0.258: locking looked like a press: freed, no click, no
                                          * card) */
            app_say("SEL", "FREE");
        } else
            btn |= 1u << B_SEL;
    }
    if (sel_now && !ui.sel_raw)
        ui.sel_t = now;                  /* (LOAF BREAKER: SEL down: held a moment, the foot shows its knobs) */
    ui.sel_raw = (uint8_t)sel_now;
    kdn = keys & ~ui.keys;
    kup = ui.keys & ~keys;
    bdn = btn & ~ui.btn;
    for (i = 0; i < 16u; i++)            /* (LOAF BREAKER: the TUTORIAL's counts: white keys, black keys) */
        tut_wk += kdn >> WK[i] & 1u;
    for (i = 0; i < 11u; i++)
        if (kdn >> BK[i] & 1u) {
            tut_pads |= 1u << i;
            tut_padn++;
        }
    int k;
    if (keys != ui.keys || btn != ui.btn)
        ui.touched = 1u;                 /* (read here too between frames: app_input_only) */
    if (ui.lobo_t || demo.on) {          /* RESET ALL's screen or the demo up: a press only ends it (app_frame) */
        for (i = 0; i < 16u; i++)        /* (but what is let go is let go: no key loop or pad left on) */
            if (kup >> WK[i] & 1u)
                brk_key((int)i, 0);
        for (i = 0; i < BRK_NPADS; i++)
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
    if (set.tut == TUT_ASK) {            /* LOAF BREAKER (0.229): the mode asked first (tutor.c): nothing else meanwhile */
        int32_t d = 0;
        for (k = 0; k < NE; k++)
            d += plat_enc(k);
        if (!ui.intro)
            ask_input(kdn, bdn, d);
        ui.keys = keys;
        ui.btn = btn;
        return;
    }
    if (tut_on() && (bdn & TUT_NAV & ~tut_btn_ok())) {   /* LOAF BREAKER (0.249): the TUTORIAL, a page button the
                                          * step doesn't ask for: held back (GLO quits) */
        bdn &= ~(TUT_NAV & ~tut_btn_ok());
        app_say("TUTORIAL", "FOLLOW THE STEP");
    }
    ovl_keys(keys, kdn);                 /* the OVERLAY page's white keys (and the latched ones, any page) */
    for (i = 0; i < 16u; i++) {          /* white keys: slices, or a page's own */
        if (ui.page == PG_OVL)
            continue;
        if (i < NDRUMS && (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() || ui.page == PG_PERC) &&
            !playing_now()) {            /* LOAF BREAKER: the KICK menu (big or classic, its EQ and FX parts too) and the
                                          * drums' pages (big or classic), stopped: keys 1-5 play the KICK, SNARE, HAT,
                                          * CLAP once each (to hear them); playing, they play slices as ever */
            if (kdn >> WK[i] & 1u) {
                midi_drum(i);
                tut_heard |= (uint8_t)(1u << i);   /* (the TUTORIAL's: heard, stopped) */
                tut_drs |= (uint8_t)(1u << i);
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
        if (ui.page == PG_SEQ && !SEQ_BRK) {   /* the SEQ page: a break step to edit, or a drum lane's steps (LOAF
                                          * BREAKER, 0.234: on its BREAK entry, the slices) */
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
    if (ui.page == PG_OVL && ui.opage == 1u) {   /* LOAF BREAKER: ARP › BLACK KEYS: a black key picks itself (what
                                          * was held lets go as ever; nothing new starts) */
        for (i = 0; i < 11u; i++)
            if (kdn >> BK[i] & 1u)
                bk_pick(i);
        for (i = 0; i < 11u; i++)
            kdn &= ~(1u << BK[i]);
    }
    for (i = 0; i < BRK_NPADS; i++) {    /* black keys: the pads, held; the last one pressed has the screen (LOAF
                                          * BREAKER: on the keys the layout gives them, app.c bk_fn) */
        if (i <= PAD_ROLL32) {           /* LOAF BREAKER: the rolls, through their THRU (the break, the kick, both) */
            if ((kdn & PAD_BIT(i))) {
                roll_key(i, 1, (btn & 1u << B_SEL) != 0u);   /* (SEL + the pad: the kick alone, this press) */
                if (btn & 1u << B_SEL)
                    ui.tap_combo = 1u;   /* (no SEL click) */
                ui.pad_focus = (int8_t)i;
                ui.pad_since = now;
            }
            if ((kup & PAD_BIT(i)) && !(keys & PAD_BIT(i)))
                roll_key(i, 0, 0);
            continue;
        }
        if (lat_bit(i) >= 0 && bk_lat() >> lat_bit(i) & 1u) {   /* (0.9.26, FLEXOR: REVERSE, FREEZE, GATE, RANDOM's
                                          * K4 LATCH: a press on, a press off) */
            if ((kdn & PAD_BIT(i))) {
                pad_latched ^= (uint16_t)(1u << i);
                brk_pad((int)i, pad_latched >> i & 1u);
                ui.pad_focus = (int8_t)i;
                ui.pad_since = now;
                app_say(PAD_TITLE[i], pad_latched >> i & 1u ? "LATCHED" : "OFF");
            }
            continue;
        }
        if (pad_latched >> i & 1u && !(keys & PAD_BIT(i))) {   /* (its LATCH turned off, let go: off) */
            pad_latched &= (uint16_t)~(1u << i);
            brk_pad((int)i, 0);
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
        if ((kup & PAD_BIT(i)) && !(keys & PAD_BIT(i)) && !(i == PAD_HALF && half_lat))
            brk_pad((int)i, 0);
    }
    if ((kdn & PAD_BIT(BKF_ARP))) {      /* GLOUPI BREAKS: the ARP key, an OVERLAY effect (ovl.c) */
        ovl_pad_key(1);
        ui.pad_focus = (int8_t)BKF_ARP;
        ui.pad_since = now;
    }
    if ((kup & PAD_BIT(BKF_ARP)) && !(keys & PAD_BIT(BKF_ARP)))
        ovl_pad_key(0);
    if ((kdn & PAD_BIT(BKF_SUB)))        /* GLOUPI BREAKS: SUB turns the kick's SUB DRIVE on and off */
        sub_drive_toggle();
    if (kdn & PAD_BIT(BKF_DST)) {        /* LOAF BREAKER: DST, a switch: the kick's DISTO used or not, its amount never
                                          * touched (0.265, FLEXOR: no more 50 % from nothing) */
        set.selk_hi ^= 16u;
        app_say("DISTO", terror_off ? "OFF" : KICK_TERROR(&set.kick) ? "ON" : "ON (AT 0 %)");
        changed_kick();
        ui.redraw |= RG_FOOT | RG_BLOCKS;
    }
    if (kdn & PAD_BIT(BKF_KIL)) {        /* LOAF BREAKER: KIL held: the sequencer's kicks quiet; let go: back. Its
                                          * LATCH (K1 while held): a press quiet, the next back */
        ui.kil_focus = 1u;
        kick_hold = set.half_mt_off & 4u ? (uint8_t)!kick_hold : 1u;
        app_say("KICKS", kick_hold ? (set.half_mt_off & 4u ? "QUIET (KIL LATCHED)" : "QUIET (KIL)") : "BACK");
        ui.redraw |= RG_FOOT;
    } else if (!(keys & PAD_BIT(BKF_KIL))) {
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
        for (i = 0; i <= BKF_ARP; i++)
            if ((keys & PAD_BIT(i)))
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
        tut_home++;                      /* (the TUTORIAL's count; its HOME step on PERFORM: it stays) */
        if (ui.page == PG_HOME || ui.page == PG_GEN || ui.page == PG_HSET) {   /* (TEST: HOME is PERFORM and
                                          * GENERATOR; 0.9.2 SETTINGS) */
            ui.home_hold = 1u;
            ui.home_combo = (uint8_t)(ui.page == PG_HOME && tut_home_stay());
        } else {
            ui.page = PG_HOME;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (ui.home_hold && !(btn & 1u << B_HOME)) {
        ui.home_hold = 0u;
        if (!ui.home_combo && (ui.page == PG_HOME || ui.page == PG_GEN || ui.page == PG_HSET)) {
            ui.page = ui.page == PG_HOME ? PG_GEN : ui.page == PG_GEN ? PG_HSET : PG_HOME;   /* (0.9.2: SETTINGS third) */
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (bdn & 1u << B_GLO && tut_on()) { /* LOAF BREAKER: the TUTORIAL running: GLO quits it, SEL + GLO skips a step */
        if (btn & 1u << B_SEL)
            tut_skip();
        else
            tut_quit();
    } else if (bdn & 1u << B_GLO && rst_confirm())
        ;                                /* (LOAF BREAKER, 0.239: RESET's five clicks done: GLO resets) */
    else if (bdn & 1u << B_GLO && ui.page == PG_GLOBAL && ui.gpage != GP_FILL && ui.gpage != GP_BREAK)
        home_back();                     /* (LOAF BREAKER, 0.9.2, FLEXOR: SYSTEM, GLO again: HOME; SELECT its parts) */
    else if (bdn & 1u << B_GLO)          /* GLOBAL: MIDI clock and Bluetooth; again, the pitch; again, BT audio */
        global_part(glo_next(1));        /* (LOAF BREAKER: SYSTEM's parts: MIDI, SETUP, EFFECTS, FX SET, RESET) */
    if (bdn & 1u << B_EDIT) {            /* LOAF BREAKER: EDIT: SOUNDS, the KICK menu; again, its next section (SOUND
                                          * ...). SEL + EDIT: the next source: KICK, DRUMS, BREAK (0.201; the MELODY
                                          * out in 0.209) */
        int kickp = ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX();
        if (btn & 1u << B_SEL) {
            ui.pbig = (uint8_t)!ui.kclassic;   /* (the drums in the view the kick has) */
            if (kickp) {
                ui.einst = DR_SNARE;
                edit_open();
            } else if (ui.page == PG_PERC)
                global_part(GP_BREAK);
            else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK) {
                ui.einst = 0u;
                kick_open();
            } else {
                ui.einst = (uint8_t)(ui.einst >= DR_SNARE && ui.einst < NDRUMS ? ui.einst : DR_SNARE);
                edit_open();
            }
        } else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK)
            app_say("BREAK", "ONE PART");    /* (the break's page: one part; SEL + EDIT the kick) */
        else if (kickp || ui.page == PG_PERC) {
            ui.edit_hold = 1u;           /* (at its release: the next section, the next drum; held: the other view) */
            ui.edit_t = now;
        } else if (ui.page == PG_SEQ && ui.seq_lane >= SL_KICK && sl_drum(ui.seq_lane) &&
                   !(tut_on() && TUT[set.tut - 1u].cond == TC_EDIT)) {   /* (LOAF BREAKER, 0.252: the TUTORIAL's
                                          * "PRESS EDIT: THE KICK", from SEQ's snare lane: the kick) */
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
        if (((ui.page == PG_KICK && !ui.kclassic) || (ui.page == PG_PERC && ui.pbig)) && ui.kview == 0u) {   /* (LOAF
                                          * BREAKER: the views in turn, the KICK's and the drums' together (0.231,
                                          * FLEXOR): RGB (REKORDBOX's or SERATO's colours: GLO EFFECTS K3), WAVE,
                                          * CLASSIC) */
            ui.kview = 1u;
            kv_sig = 0u;
            pw_sig = 1u;
        } else {
            ui.kclassic ^= 1u;           /* (LOAF BREAKER: on the drums' pages too: one view for them all) */
            if (ui.page == PG_PERC)
                ui.kclassic = ui.pbig;
            ui.pbig = (uint8_t)!ui.kclassic;
            if (!ui.kclassic) {
                ui.kview = 0u;
                kv_sig = 0u;
                pw_sig = 1u;
            }
        }
        app_say(ui.page == PG_PERC ? PERC_VOICE_NAME[ui.pvoice % PERC_VOICES] : "KICK",
                ui.kclassic ? "CLASSIC VIEW" : ui.kview ? "WAVE VIEW" :
                set.wstyle ? "RGB VIEW (SERATO)" : "RGB VIEW (REKORDBOX)");
        ui.redraw |= RG_BLOCKS | RG_WAVE | RG_FOOT;
        if (!ui.kclassic)
            lcd_fill(0, 44, 240, 44, C_BLACK);
    }
    if (ui.edit_hold && !(btn & 1u << B_EDIT)) {
        if (ui.edit_hold == 1u && !set.novice && ui.einst >= DR_CLAP && ui.page == PG_PERC)
            kick_open();                 /* (LOAF BREAKER, 0.266, FLEXOR: EXPERT, EDIT goes round the sounds, KICK SNARE
                                          * HAT CLAP; SELECT the KICK's sections. After the CLAP, the KICK) */
        else if (ui.edit_hold == 1u && (ui.page == PG_PERC || !set.novice)) {   /* (a drum: the next one; EXPERT's
                                          * KICK: the SNARE; BEGINNER, SNARE HAT CLAP round, FLEXOR 0.227) */
            ui.einst = (uint8_t)(ui.page != PG_PERC ? DR_SNARE : ui.einst >= DR_CLAP ? DR_SNARE : ui.einst + 1u);
            ui.pbig = (uint8_t)!ui.kclassic;   /* (in the view the kick has) */
            edit_open();
        } else if (ui.edit_hold == 1u)
            kick_sect_next();            /* (BEGINNER's KICK: KICK, SHAPE, MIX) */
        ui.edit_hold = 0u;
        ui.redraw |= RG_FOOT | RG_BLOCKS;
    }
    if (bdn & 1u << B_LFO)               /* LOAF BREAKER: LFO acts as it comes up, unless K1-K4 turned meanwhile (LFO +
                                          * K1-K4: the four levels, on any page) */
        ui.lfo_hold = 1u;
    if (ui.lfo_hold && !(btn & 1u << B_LFO)) {   /* LFO: the MIXER (the break, the kick, FX main, FX overlay); again,
                                          * groove */
        if (ui.lfo_hold == 1u && ui.page == PG_MIX)
            home_back();                 /* (0.9.2, FLEXOR: MIX, LFO again: HOME; SELECT its tabs) */
        else if (ui.lfo_hold == 1u) {
            ui.mpage = 0u;               /* (LEVELS, SENDS, MASTER, DYNAMICS; BEGINNER LEVELS and SENDS alone) */
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
        } else if (ui.page == PG_TRACK)
            home_back();                 /* (0.9.2, FLEXOR: SONG, ENV again: HOME; SELECT its tabs) */
        else {
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
            home_back();                 /* (0.9.2: TRACKS, ENV again (not held for a gap): HOME) */
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
            ui.seq_brk = 0u;
            ui.page = PG_SEQ;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (seq_wait) {
        if (!(btn & 1u << B_SEQ)) {      /* let go: on SEQ already, HOME (0.9.2, FLEXOR: SELECT the lanes) */
            if (seq_wait & 2u)
                home_back();
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
    if (bdn & 1u << B_ARP && btn & 1u << B_SEL && (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX()))
        kick_clip();                     /* (LOAF BREAKER, 0.9.26, FLEXOR: in the KICK LAB, SEL + ARP copies the kick, on
                                          * another kick pastes it) */
    else if (bdn & 1u << B_ARP && btn & 1u << B_SEL && ui.page == PG_PERC)
        perc_clip();                     /* (the drums: the drum's sound, onto another memory of it) */
    else if (bdn & 1u << B_ARP && btn & 1u << B_SEL) {   /* LOAF BREAKER: SEL + ARP: PATTERN's GROOVE (its SWING) */
        global_part(GP_FILL);
    } else if (bdn & 1u << B_ARP) {      /* ARP: the OVERLAY page (LOAF BREAKER, 0.244, FLEXOR: back, FILL gone); on it
                                          * again, EXPERT: its next tab (ARP EFFECTS, BLACK KEYS, MORE) */
        if (ui.page == PG_OVL)
            home_back();                 /* (0.9.2, FLEXOR: OVERLAY, ARP again: HOME; SELECT its tabs) */
        else {
            slices_let_go(keys);
            ui.opage = 0u;
            ui.page = PG_OVL;
            ui.redraw |= RG_HEAD;
            ui.redraw |= RG_FOOT | RG_BLOCKS;
            page_say();
        }
    }
    if (bdn & 1u << B_FX && ui.pad_focus == BKF_ARP) {   /* LOAF BREAKER: FX with the ARP key held: its knobs, MAIN
                                          * <-> MORE */
        ui.pad_more ^= 1u;
        app_say("ARP KEY", ui.pad_more ? "MORE" : "MAIN");
        ui.redraw |= RG_BLOCKS;
        foot_dirty = 1u;
    } else if (bdn & 1u << B_FX && key_tuned() >= 0 && !set.novice) {   /* FX with a key held: its knobs, loop <->
                                          * slice effect (LOAF BREAKER: EXPERT's) */
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
    } else if (bdn & 1u << B_FX) {       /* FX: the chain's page; again (let go alone): the slot's next part (LOAF BREAKER:
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
    if (ui.fx_hold && !(btn & 1u << B_FX)) {   /* FX let go: alone, the page or the slot's next part; with ALGORITHM turned, a
                                          * move */
        ui.fx_hold = 0u;
        if (!ui.fx_turned) {
            if (ui.fx_open) {
                kfx_open(0);
                ui.page = PG_FX;
                page_say();
                ui.redraw |= RG_FOOT | RG_BLOCKS;
            } else if (ui.page == PG_FX && IS_KFX())
                fx_next_part();          /* (KICK FX: the slot's next part, round: MAIN, MORE, MOD) */
            else if (ui.page == PG_FX)
                home_back();             /* (0.9.2, FLEXOR: EFFECTS, FX again: HOME; SELECT and ALGORITHM move in it) */
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
        tut_saves++;
        int ok = store_all();
        if (ok)
            ab_take();                   /* (TOOLS' A/B: B is the kick as now saved) */
        app_say(pre ? KPRE_NAME[kstyle_at - KSTYLES] : "SAVE", ok ? (pre ? "STORED, SAVED" : "DONE")
                : wiped ? "OFF: MEMORY WIPED" : "FAILED");   /* (0.263: wiped, nothing saved until a restart) */
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
        tut_heard = 0u;                  /* (LOAF BREAKER: a knob turned: heard again only after it) */
        if (k >= EN_K1 && k <= EN_K4) {  /* (LOAF BREAKER: the TUTORIAL's counts: the kick's knobs, SEL + K4) */
            if (btn & 1u << B_SEL)
                tut_sk4 += k == EN_K4 ? (uint32_t)(d < 0 ? -d : d) : 0u;
            else if (ui.page == PG_KICK)
                tut_kturn += (uint32_t)(d < 0 ? -d : d);
        }
        if (btn & 1u << B_SEL)
            ui.tap_combo = 1u;           /* (SEL + a knob is no tap) */
        if (ui.fx_hold)
            ui.fx_turned = 1u;           /* (FX + a knob: not the next part) */
        if (!(btn & 1u << B_SEL))
            ui.help = 0u;                /* (a knob closes the card) */
        if (k == EN_SELECT && btn & 1u << B_SEL) {   /* LOAF BREAKER: SEL + SELECT, on any page: the drums' filter
                                          * (DRUM FLT, HOME's K3; was the kick's memories, now PRESETS on the KICK menu) */
            kick_flt_turn(d);
            continue;
        }
        if (k >= EN_K1 && k <= EN_K4 && btn & 1u << B_LFO) {   /* LOAF BREAKER: LFO + K1 the break's level, K2 the
                                          * kick's, K3 the drums', on any page (the MIXER's LEVELS: the page
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
            ui.redraw |= RG_FOOT;        /* (the foot shows them: SEL held or locked) */
            continue;
        }
        if (k == EN_SELECT && !set.novice && (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX())) {
            kick_sect_turn(d);           /* LOAF BREAKER (0.264, FLEXOR): EXPERT's KICK menu, SELECT the tabs at the top
                                          * (its sections), ALGORITHM those at the bottom (its parts); the drums: SEL +
                                          * EDIT */
            ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
            continue;
        }
        if (k == EN_SELECT && (ui.page == PG_KICK || ui.page == PG_PERC) &&
            (set.novice || (ui.page == PG_KICK && !ui.kclassic) || (ui.page == PG_PERC && ui.pbig))) {   /* (BEGINNER:
                                          * either view, 0.230) */
            if (!set.novice && ui.page == PG_PERC)   /* (0.264, FLEXOR: EXPERT's drums, SELECT their tabs at the top,
                                          * SNARE, HAT, CLAP round; the KICK: SEL + EDIT) */
                ui.einst = (uint8_t)(1u + (ui.einst + (d > 0 ? 0u : 1u)) % (NDRUMS - 1u));
            else
            ui.einst = (uint8_t)((ui.einst + (d > 0 ? 1u : NDRUMS - 1u)) % NDRUMS);   /* LOAF BREAKER: the big view:
                                          * SELECT goes from drum to drum (the KICK, SNARE, HAT, CLAP), each in
                                          * it (not the tempo here) */
            if (ui.einst) {
                ui.pbig = (uint8_t)!ui.kclassic;   /* (0.231: the view the kick has) */
                edit_open();
            } else
                kick_open();
            ui.redraw |= RG_BLOCKS | RG_FOOT;
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_FX && !IS_KFX()) {   /* LOAF BREAKER (0.9.2, FLEXOR): EFFECTS, SELECT the
                                          * slots (the tabs at the top), ALGORITHM their parts (MAIN, MORE, MOD) */
            fx_algo(d, 0u);
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_TRACK) {   /* (0.9.2, FLEXOR: SONG, SELECT its tabs; the tempo: HOME) */
            track_tab_turn(d);
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_SEQ) {   /* (0.9.31, FLEXOR: PATTERN, SELECT its views: FULL, DRUMS,
                                          * SETTINGS; ALGORITHM the lanes) */
            seq_view_turn(d);
            page_say();
            ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_GLOBAL && ui.gpage == GP_FILL) {   /* (SETTINGS: on round to FULL, back to
                                          * DRUMS) */
            ui.page = PG_SEQ;
            seq_view_set(d > 0 ? 0u : 1u);
            page_say();
            ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_MIX) {   /* (0.9.2, FLEXOR: MIX, SELECT its tabs: LEVELS, SENDS, MASTER,
                                          * DYNAMICS; BEGINNER the first two) */
            uint32_t n = set.novice ? 2u : 4u;
            ui.mpage = (uint8_t)((ui.mpage % n + (d > 0 ? 1u : n - 1u)) % n);
            ui.redraw |= RG_HEAD | RG_FOOT | RG_BLOCKS;
            page_say();
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_GLOBAL && ui.gpage != GP_FILL && ui.gpage != GP_BREAK) {   /* (0.9.2,
                                          * FLEXOR: SYSTEM, SELECT its parts: MIDI, SETUP, EFFECTS, FX SET, RESET) */
            global_part(glo_next(d));
            continue;
        }
        if (k == EN_SELECT && ui.page == PG_OVL && !set.novice) {   /* (0.9.2, FLEXOR: OVERLAY, SELECT its tabs) */
            ui.opage = (uint8_t)((ui.opage + 1u) % 2u);
            page_say();
            ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
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
            else if (btn & 1u << B_SEL && sel_bt_page())
                hset_turn(3, d);         /* (0.9.26, FLEXOR: where ALGORITHM alone is the break, SEL + ALGORITHM
                                          * BREAK TIME, right HALF, left NORMAL) */
            else if (btn & 1u << B_SEL)
                break_ask((set.brk + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks);
            else if (ui.page == PG_OVL && ui.opage == 1u)
                bk_turn(1, d);           /* (0.9.26, FLEXOR: ARP › BLACK KEYS, ALGORITHM the key, as K2) */
            else if (ui.page == PG_TRACK && ui.tpage == 0u)
                track_turn(-1, d);       /* LOAF BREAKER: ENV's TRACKS: the item (the break: SEL + ALGORITHM; K1 the
                                          * item's kick) */
            else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK)   /* LOAF BREAKER: SOUNDS' BREAK: the break */
                break_ask((set.brk + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks);
            else if (ui.page == PG_GLOBAL && ui.gpage == GP_FILL) {   /* PATTERN's GROOVE: back to SEQ's lanes (left:
                                          * the last) */
                ui.page = PG_SEQ;
                ui.seq_manual = 1u;
                ui.seq_lane = (uint8_t)(d > 0 ? SL_BREAK1 : seq_last_lane());
                ui.seq_brk = 0u;
                if (!lane_ok(ui.seq_lane))
                    seq_lane_step(1);
                page_say();
                ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_GLOBAL)   /* GLOBAL: its part (LOAF BREAKER: SYSTEM's) */
                global_part(glo_next(d));
            else if (ui.page == PG_KICK) {    /* LOAF BREAKER: the KICK menu: the next part of its section */
                kick_sect_step(d);
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_SEQ) {   /* SEQ: the lane its keys edit */
                ui.seq_manual = 1u;      /* (LOAF BREAKER: picked: not following the bar playing, until stopped) */
                seq_lane_step(d);        /* (0.9.31, FLEXOR: every lane, top to bottom, round) */
                page_say();
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_KEQ) {   /* (the KICK menu's EQ: MORE's next part; FX1, FX2: their own parts, as the
                                          * FX page's, below; 0.9.29, FLEXOR) */
                kick_sect_step(d);
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_PERC) {   /* the snare or the hat: SOUND or SHAPE (LOAF BREAKER: MORE; BEGINNER:
                                          * its one page) */
                ui.ppage = (uint8_t)((ui.ppage + (d > 0 ? 1u : 2u)) % 3u);
                page_say();
                ui.redraw |= RG_BLOCKS | RG_FOOT;
            } else if (ui.page == PG_FX)
                fx_part_turn(d);         /* (0.9.2: the slot's parts; SELECT the slots) */
            else {                     /* the next or previous break */
                break_ask((set.brk + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks);
            }
        } else if (k == EN_PRESET && (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX())) {
            if (IS_KFX() && !(btn & 1u << B_SEL))   /* (0.9.29, FLEXOR: FX1, FX2, PRESETS the effects alone; SEL +
                                          * PRESETS the kick there) */
                fx_type_turn(d);
            else if (IS_KFX() || (ui.page == PG_KEQ && btn & 1u << B_SEL))
                kick_mem_turn(d);
            else if (ui.page == PG_KEQ)
                ;                        /* (0.9.29, FLEXOR: EQ B, EQ S, PRESETS nothing, SEL + PRESETS the kick) */
            else if (!(btn & 1u << B_SEL))   /* (LOAF BREAKER, 0.9.26, FLEXOR: in the KICK menu, PRESETS the kick on every
                                          * page; SEL + PRESETS what PRESETS did there: K1-K4 SET a knob's function,
                                          * ASSIGN a style; elsewhere the pattern) */
                kick_mem_turn(d);
            else if (ui.page == PG_KICK && ui.kpage == KP_KSET)
                selk_assign_k(ui.kks, d);
            else if (ui.page == PG_KICK && ui.kpage == KP_STYLE)
                kstyle_turn(d);
            else
                pattern_pick(d);
            ui.redraw |= RG_FOOT;
        } else if (k == EN_PRESET && btn & 1u << B_SEL &&
                   (ui.page <= PG_GEN || ui.page == PG_HSET || ui.page == PG_SEQ || ui.page == PG_MIX ||
                    ui.page == PG_OVL || (ui.page == PG_GLOBAL && ui.gpage != 2u && ui.gpage != 3u))) {
            kick_mem_turn(d);            /* (LOAF BREAKER, 0.9.26, FLEXOR: on HOME, SEL + PRESETS the kick, KICK 1-20;
                                          * PRESETS alone the pattern) */
            ui.redraw |= RG_FOOT;
        } else if (k == EN_PRESET && btn & 1u << B_SEL)
            pattern_pick(d);             /* SEL held: the pattern, on any page (HOME: the kick, above) */
        else if (k == EN_PRESET && ui.page == PG_FX)
            fx_type_turn(d);   /* the focused slot's effect (LOAF
                                          * BREAKER: BEGINNER, a whole scene) */
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
        } else if (k == EN_PRESET && ui.page == PG_TRACK && ui.tpage == 2u) {   /* LOAF BREAKER: DROP's
                                          * BUILD KICK (BEGINNER too, 0.9.22) */
            char t[12];
            set.drop_kick = (uint8_t)clamp((int32_t)set.drop_kick + (d > 0 ? 1 : -1), 0, (int32_t)KICK_MEMS);
            t[0] = 0;
            drop_kick_text(t);
            set_dirty = 1u;
            app_say("BUILD KICK", t);
            ui.redraw |= RG_BLOCKS;
        } else if (k == EN_PRESET && ui.page == PG_TRACK && ui.tpage < 2u)
            track_pick(d);               /* LOAF BREAKER: ENV: the track */
        else if (k == EN_PRESET && ui.page == PG_TRACK && ui.tpage == 4u) {   /* LOAF BREAKER TEST:
                                          * BUILD's TEMPO RISE */
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
        else if (k == EN_PRESET && ui.page == PG_GLOBAL && ui.gpage == 2u)
            global_turn(0, d);           /* (SETUP: PRESETS the skin, as K1; 0.9.26, FLEXOR: WAVE ZOOM before) */
        else if (k == EN_PRESET)
            pattern_pick(d);             /* elsewhere: the pattern */
        else if (ui.kil_focus && k == EN_K1) {   /* LOAF BREAKER: KIL held, K1: its LATCH, right on (held now: it
                                          * stays quiet), left off */
            set.half_mt_off = (uint8_t)(d > 0 ? set.half_mt_off | 4u : set.half_mt_off & ~4u);
            if (d > 0)
                kick_hold = 1u;
            set_dirty = 1u;
            app_say("KIL LATCH", d > 0 ? "ON" : "OFF");
            ui.redraw |= RG_FOOT;
        } else if (key_tuned() >= 0) {   /* a white key held: K1-K4 tune its loop (LOAF BREAKER: BEGINNER, not) */
            if (!set.novice)
                key_knob_turn(key_tuned(), k - EN_K1, d);
            else
                easy_no();               /* (0.229: said) */
        } else if (ui.pad_focus >= 0)    /* a pad held: K1-K4 tune it (LOAF BREAKER, 0.232: BEGINNER too) */
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
                                             B_EDIT, B_HOME};   /* (TEST: KICK EQ under EDIT) */
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
    for (i = 0; i < BKF_N; i++)          /* (LOAF BREAKER: each function lit on its keys) */
        if (bkf_lit(i, &st))
            k |= PAD_BIT(i);
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
    tut_leds(&b, &k, st.playing);        /* (LOAF BREAKER: the TUTORIAL's step: its buttons and keys blinking) */
    plat_glow(1);
    plat_leds(b, k);
}

/* ------------------------------------------------------------ slicer view --- */
static void knob_value(int i, char *t)  /* the page's knob i (0..3), as shown under its name */
{
    t[0] = 0;
    if (ui.page == PG_OVL) {             /* (LOAF BREAKER, 0.244: the OVERLAY page) */
        ovl_value(i, t);
        return;
    }
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
    else if (ui.page == PG_HSET)
        hset_value(i, t);
    else if (ui.page == PG_GEN) {
        float v = i == 0 ? set.gen.complexity : i == 1 ? set.gen.anchor : i == 2 ? set.gen.roll : set.gen.fill;
        int_text((int32_t)(v * 100.0f), t);
        text_cat(t, "%");
    } else
        fx_value(ui.fx_focus, i, t);
}

/* the tab bar under the header: the button's name, then the page's parts as tabs (the one you are on lit: the
 * button again, or ALGORITHM, goes to the next); EDIT and FX also name the part edited, "< SUB FX >" */
static const char *const OPAGE_NAME[3] = {"ARP EFFECTS", "BLACK KEYS", "MORE"};   /* (LOAF BREAKER: ARP's tabs) */
static int32_t tab(int32_t x, const char *s, int on)
{
    int32_t w = text_w(&FONT_T, s) + 8;
    cv_rect(x, 0, w, 13, on ? C_HI : C_BLOCK);
    cv_text(x + 4, 1, &FONT_T, s, on ? C_BLACK : C_GRAY);
    return x + w + 3;
}
/* the pages' buttons as printed, GLOBAL's parts, SEQ's lanes: for the tabs and the header (page_say) */
static const char *const BTN[NPAGES] = {"HOME", "HOME", "FX", "GLO", "EDIT", "LFO", "SEQ", "EDIT", "ENV", "ARP",
                                        "EDIT", "HOME"};
static const char *const GPART[NGPART] = {"MIDI", "PITCH", "SETUP", "EFFECTS", "FX SET", "RESET"};
/* LOAF BREAKER (0.201): each button a domain, its name on the screen (the button keeps its print): LIVE (HOME), SOUNDS
 * (EDIT: the kick, the drums, the melody, the break), PATTERN (SEQ), SONG (ENV), MIX (LFO), EFFECTS (FX), SYSTEM (GLO) */
static const char *const DOM[NPAGES] = {"LIVE", "LIVE", "EFFECTS", "SYSTEM", "SOUNDS", "MIX", "PATTERN", "SOUNDS", "SONG",
                                        "OVERLAY", "SOUNDS", "LIVE"};
static const char *dom_name(void)
{
    if (IS_KFX() || (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK))
        return "SOUNDS";
    if (ui.page == PG_GLOBAL && ui.gpage == GP_FILL)
        return "PATTERN";
    return DOM[ui.page % NPAGES];
}
static const char *const SEQL[NSEQL] = {"1-16", "17-32", "K1", "K2", "K3", "K4", "S1", "S2", "S3", "S4", "H1", "H2", "H3", "H4", "C1", "C2", "C3", "C4",
};
static void draw_tabs(void)
{
    uint32_t i;
    int32_t x;
    char part[20];
    int sel_hint = 0;                    /* (LOAF BREAKER, 0.230: BEGINNER, "SELECT: ..." at the right) */
    part[0] = 0;
    cv_begin(240, 14, C_BLACK);
    {                                    /* (LOAF BREAKER: the domain; SOUNDS: the source, KICK, DRUMS, BREAK) */
        const char *lab = ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() ? "KICK" : ui.page == PG_PERC
                          ? "DRUMS" : ui.page == PG_GLOBAL && ui.gpage == GP_BREAK
                          ? "BREAK" : dom_name();
        x = ui.page == PG_TRACK || ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() ? 2   /* (LOAF BREAKER: the
                                          * KICK menu's six sections want the room; its picture says KICK n) */
            : cv_text(4, 1, &FONT_T, lab, C_WHITE) + 6;
    }
    if (ui.page == PG_HOME || ui.page == PG_GEN || ui.page == PG_HSET) {   /* HOME: PERFORM, GENERATOR, SETTINGS */
        x = tab(x, "PERFORM", ui.page == PG_HOME);
        x = tab(x, "GENERATOR", ui.page == PG_GEN);
        tab(x, "SETTINGS", ui.page == PG_HSET);
    }
    else if (ui.page == PG_OVL) {        /* (LOAF BREAKER: EXPERT, its three tabs; BEGINNER, ARP EFFECTS alone: no tab) */
        if (!set.novice)
            for (i = 0; i < 2u; i++)     /* (0.259, FLEXOR: MORE gone for now) */
                x = tab(x, OPAGE_NAME[i], i == ui.opage);
    }
    else if (ui.page == PG_TRACK)        /* LOAF BREAKER: ENV: TRACKS, PLAY, DROP */
        for (i = 0; i < NTORD; i++)   /* (TRACKS, PLAY, DROP, BUILD: no "ENV" before them, for the room) */
            x = tab(x, TPART[TORDER[i]], TORDER[i] == ui.tpage);
    else if (ui.page == PG_GLOBAL && ui.gpage == GP_BREAK)   /* (SOUNDS' BREAK: the break playing) */
        tab(x, break_name(set.brk), 1);
    else if (ui.page == PG_GLOBAL && ui.gpage != GP_FILL)   /* (SEQ's SETTINGS: SEQ's tabs, below) */
        for (i = 0; i < NGLO; i++)
            x = tab(x, GPART[GLO_ORD[i]], GLO_ORD[i] == ui.gpage);
    else if (ui.page == PG_MIX) {
        x = tab(x, "LEVELS", !ui.mpage);
        x = tab(x, "SENDS", ui.mpage == 1u);
        x = tab(x, "MASTER", ui.mpage == 2u);
        tab(x, "DYN", ui.mpage == 3u);   /* (LOAF BREAKER: COMP's and DUCK's timing, what DUCK ducks) */
    } else if (ui.page == PG_SEQ || (ui.page == PG_GLOBAL && ui.gpage == GP_FILL)) {
        char pn[6] = "P";                /* LOAF BREAKER: the pattern, then SELECT's views (0.9.31, FLEXOR: FULL, DRUMS,
                                          * SETTINGS); the lane the keys edit in the header */
        uint32_t v = ui.page == PG_GLOBAL ? 2u : brk_view() ? 0u : 1u;
        int_text((int32_t)pat_cur() + 1, pn + 1);
        if (pat_next >= 0)
            text_cat(pn, ">");
        x = tab(x, pn, 0);
        x = tab(x, "FULL", v == 0u);
        x = tab(x, "DRUMS", v == 1u);
        tab(x, "SETTINGS", v == 2u);
    } else if (ui.page == PG_FX && !IS_KFX()) {
        for (i = 0; i < fx_nslots(); i++)   /* (LOAF BREAKER: BEGINNER's two) */
            x = tab(x, SLOT_NUM[i], i == fx_foc());
        text_cat(part, fx_name(fx_foc()));
        if (ui.fx_page) {                /* "< MB COMP X-OVER >", "< FILTER MOD >" */
            text_cat(part, " ");
            text_cat(part, fx_part_name());
        }
    } else if (ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX()) {   /* LOAF BREAKER: the KICK menu: its sections,
                                          * the part (if it fits) */
        if (kick_easy()) {               /* (LOAF BREAKER: BEGINNER's KICK: the sounds, SELECT going round them; its
                                          * pages are the foot's tabs, 0.230, FLEXOR) */
            for (i = 0; i < NDRUMS; i++)
                x = tab(x, DRUM_TAG[i], i == 0u);
            sel_hint = 1;
        } else {
            for (i = 0; i < KSECTS; i++)
                x = tab(x, KSECT_NAME[i], i == ui.ksect % KSECTS);
            kick_part_name(kick_part(), part);
        }
        if (x + text_w(&FONT_T, part) + 16 > 236)
            part[0] = 0;
    } else {                             /* SOUNDS: the drum, and its part */
        for (i = set.novice ? 0u : 1u; i < NDRUMS; i++)
            x = tab(x, DRUM_TAG[i], i == ui.einst);   /* (SN HH CP; BEGINNER: KICK first, SELECT round them) */
        if (set.novice)
            sel_hint = 1;
        else
            text_cat(part, PPAGE_NAME[perc_pg()]);
    }
    if (sel_hint)                        /* (LOAF BREAKER, 0.230: BEGINNER's sounds, SELECT goes round them) */
        cv_text(236 - text_w(&FONT_T, "SELECT: KICK SN HH CP"), 1, &FONT_T, "SELECT: KICK SN HH CP", C_DIM);
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
    case PG_HSET: text_cat(t, "SETTINGS"); break;
    case PG_KEQ: text_cat(t, ui.keq_part ? "KICK EQ SHAPE" : "KICK EQ BANDS"); break;
    case PG_GLOBAL: text_cat(t, ui.gpage == GP_FILL ? "SETTINGS" : ui.gpage == GP_BREAK ? "BREAK" : GPART[ui.gpage % NGPART]); break;
    case PG_MIX: text_cat(t, ui.mpage == 3u ? "DYNAMICS" : ui.mpage == 2u ? "MASTER" : ui.mpage ? "SENDS" : "LEVELS"); break;
    case PG_SEQ: text_cat(t, SEQ_BRK ? "BREAK" : SEQ_LANE_NAME[ui.seq_lane % NSEQL]); break;
    case PG_TRACK: text_cat(t, TPART[ui.tpage % NTPART]); break;
    case PG_OVL: text_cat(t, OPAGE_NAME[ui.opage % 3u]); break;   /* (LOAF BREAKER: its tabs) */
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
        text_cat(t, " ");
        text_cat(t, PPAGE_NAME[perc_pg()]);
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
        {{"EDIT HELD", "BIG / CLASSIC VIEW"}, {"EDIT", "NEXT SOUND"}, {"PRESETS", "KICK 1-20"},
         {"SELECT", "NEXT SECTION"}},   /* (0.9.2: EDIT round the sounds (0.266), SELECT the sections (0.264)) */
        {{"EDIT HELD", "BIG / CLASSIC VIEW"}, {"EDIT", "NEXT SOUND"}, {"PRESETS", "MEMORY / INSTRUMENT"},
         {"SELECT", "NEXT DRUM"}},
        {{"SEQ + KEY 1-4", "COPY THE BAR THERE"}, {"SEQ + K1", "SHIFT THE LANE"}, {"SEQ + REC", "CLEAR THE DRUMS"},
         {"+ SAVE", "UNDO"}},
        {{"+ FX", "BYPASS SLOT"}, {"FX + ALGO", "MOVE SLOT"}, {"+ REC", "RESET EFFECT"}, {"PRESETS", "EFFECT TYPE"}},
        {{"+ PLAY", "START ITEM (TRACKS)"}, {"PLAY", "PLAY THE TRACK"}, {"PRESETS", "THE TRACK"},
         {"WHITE KEYS", "PATTERNS / TRACKS"}},
        {{"LFO + K1", "BREAK LEVEL"}, {"LFO + K2", "KICK LEVEL"}, {"LFO + K3", "DRUMS LEVEL"}, {"LFO + K", "LEVELS ANYWHERE"}},
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
        cv_text(46, (int32_t)i * 13, &FONT_XS, i != 3u ? G[i][1] : ui.page == PG_PERC ? "COPY DRUM"
                : ui.page == PG_KICK || ui.page == PG_KEQ || IS_KFX() ? "COPY KICK" : G[i][1], C_WHITE);
        cv_text(124, (int32_t)i * 13, &FONT_XS, G[i + 4u][0], C_HI);
        cv_text(170, (int32_t)i * 13, &FONT_XS, i == 2u && sel_bt_page() ? "BREAK TIME" : G[i + 4u][1], C_WHITE);
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
               : -1.0f;
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
                                              {"BREAK", "KICK", "PERC", ""},
                                              {"SLICE", "PITCH", "HITS", "REVERSE"},
                                              {"", "", "", ""},
                                              {"KICK", "PUT", "LOOPS", "BREAK"},
                                              {"AMOUNT", "RATE", "SWEEP", "LATCH"},
                                              {"LOW", "MID", "HIGH", "MID FREQ"},
                                              {"VIEW", "WAVE TYPE", "WAVE ZOOM", "BREAK TIME"}};
    uint32_t i;
    char t[16];
    draw_tabs();
    cv_begin(240, 48, C_BLACK);
    for (i = 0; i < 11u; i++) {          /* (LOAF BREAKER: each key's function, the layout's) */
        uint32_t f = bk_fn(i);
        int on = bkf_lit(f, st);
        int32_t x = 2 + (int32_t)i * 21;
        cv_rect(x, 0, 20, 14, on ? C_HI : C_BLOCK);
        cv_text(x + (20 - text_w(&FONT_T, BKF_NAME[f])) / 2, 1, &FONT_T, BKF_NAME[f], on ? C_BLACK : C_GRAY);
    }
    for (i = 0; i < 4u; i++) {
        static const char *const KN_PITCH[4] = {"PITCH", "GRAIN", "SNAP", "LEVEL"};   /* (SOUNDS' BREAK) */
        static const char *const KN_GROOVE[3][4] = {{"FX", "DRUMS", "ARP", ""}, {"COMP", "RATIO", "DUCK", "MAX OUT"},
                                                    {"C ATTACK", "C RELEASE", "D RELEASE", "DUCKS"}};
        static const char *const KN_MORE[3][4] = {{"SKIN", "", "MODE", "AUTO SAVE"},   /* SETUP (K2 WAVE ZOOM: HOME's
                                          * SETTINGS, 0.9.2) */
                                                  {"EFFECT", "ON / OFF", "", ""},   /* (LOAF BREAKER: EFFECTS, */
                                                  {"AMOUNT", "KICK", "DROP", "LENGTH"}};   /* FX SET) */
        static const char *const KN_TRK[4][4] = {{"TRACK", "FROM", "END", ""}, {"BUILD", "AFTER", "BREAK", "CRASH"},
                                                 {"STYLE", "SWEEP", "RISER", "GAP"}, {"", "RESET", "", "WHAT"}};
        static const char *const KN_FILL[4] = {"SWING", "BREAK ROWS", "FOLLOW", ""};   /* (LOAF BREAKER: GROOVE; 0.244 FILL gone) */
        static const char *const KN_BK[2][4] = {{"PRESET", "KEY", "EFFECT", ""}, {"", "", "", ""}};   /* (ARP's
                                          * BLACK KEYS, MORE) */
        const char *name = ui.page == PG_OVL && ui.opage ? KN_BK[(ui.opage - 1u) & 1u][i] :
                           ui.page == PG_GLOBAL && ui.gpage == GP_FILL ? KN_FILL[i] : RST_ON ? KN_TRK[3][i] : ui.page == PG_TRACK &&
                           ui.tpage ? KN_TRK[ui.tpage == 4u ? 2u : (ui.tpage - 1u) & 1u][i] : ui.page == PG_GLOBAL &&
                           ui.gpage >= 2u ? KN_MORE[(ui.gpage - 2u) % 3u][i] : ui.page == PG_GLOBAL && ui.gpage
                           ? KN_PITCH[i] : ui.page == PG_MIX && ui.mpage
                           ? KN_GROOVE[ui.mpage >= 3u ? 2u : ui.mpage == 2u][i] : KN[ui.page][i];
        if (ui.page == PG_TRACK && !ui.tpage && !RST_ON && i >= 2u && trk_now()->it[ui.trk_cur % TRK_ITEMS] == TI_DROP)
            name = i == 2u ? "STUTTER" : "GAP KEEPS";   /* (0.9.26: a DROP item's K3, K4) */
        if (ui.sel_foot) {               /* LOAF BREAKER (0.258, FLEXOR): SEL held or locked (SEL x2): the knobs are
                                          * SEL + K1-K4's, their functions (EDIT KICK › KNOBS) and their values */
            selk_value(selk_fn(i), 0, t);
            cv_text(4 + (int32_t)i * 60, 18, &FONT_T, SK_NAME[selk_fn(i)], C_AMB);
            if (text_w(&FONT_XS, t) > 54)
                cv_text(2 + (int32_t)i * 60, 31, &FONT_T, t, C_WHITE);
            else
                cv_text(4 + (int32_t)i * 60, 30, &FONT_XS, t, C_WHITE);
            continue;
        }
        if (ui.shown >= 100 && set.novice)
            continue;                    /* (LOAF BREAKER: BEGINNER, a key held: no knob) */
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
            name = perc_kname(ui.pvoice, perc_pg(), (int)i);
        else if (ui.page == PG_KEQ)
            name = KEQ_NAME[ui.keq_part & 1u][i];
        else if (ui.page == PG_SEQ && SEQ_BRK)
            name = i == 3 ? "STEPS" : "";   /* (LOAF BREAKER, 0.234: BREAK, K4 its rows shown) */
        else if (ui.page == PG_SEQ && ui.seq_lane >= SL_KICK) {
            static const char *const KN_LANE[2][4] = {{"FILL", "SOUND", "CLR / 4X4", "LENGTH"},
                                                      {"NOTE", "HITS", "COND", "NUDGE"}};   /* (LOAF BREAKER: a
                                          * step held, its own; K2 what the lane plays: KICK, the drum's SOUND) */
            name = KN_LANE[ui.note_key != 0u][i];
            if (!ui.note_key && i == 1)
                name = sl_kick(ui.seq_lane) ? "KICK" : "SOUND";
            if (set.novice)              /* (LOAF BREAKER: BEGINNER, no step menu) */
                name = KN_LANE[0][i];
            if (set.novice && i == 1)
                name = sl_kick(ui.seq_lane) ? "KICK" : "SOUND";
        }
        if (ui.page == PG_GLOBAL && !ui.gpage && i == 2u && !set.novice)
            name = "METER";              /* (LOAF BREAKER, 0.213: SYSTEM › MIDI K3, EXPERT only) */
        if (!trk_knob_ok((int)i) || !glo_knob_ok((int)i))   /* (LOAF BREAKER: BEGINNER, a knob it leaves out: blank) */
            continue;
        cv_text(4 + (int32_t)i * 60, 18, &FONT_T, name, ui.page == PG_FX
                ? (fx_sl(fx_foc())->on ? C_HI : C_DIM) : C_GRAY);
        knob_value((int)i, t);
        if (text_w(&FONT_XS, t) > 54)    /* (LOAF BREAKER, 0.238: a value wider than its column, GLUCKSMAN: the
                                          * smaller letters, from 2 px to the left, not into the next; 0.239: 54) */
            cv_text(2 + (int32_t)i * 60, 31, &FONT_T, t, C_WHITE);
        else
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
    ui.kpreset = (uint8_t)kpre_mem(set.kslot);   /* (0.9.2: the PRESET page names the kick's own) */
    ui.blocks = (uint8_t)(set.bkeys.bk[BK_VIEW] & 1u);   /* (0.9.2: HOME's VIEW as it was left) */
    ui.ksect = 0u;                       /* (LOAF BREAKER, 0.253, FLEXOR: the KICK menu opens on ATTACK's PRESET, the
                                          * built-in kicks first; 0.205-0.252 on BODY, its MIX) */
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
    wipe_step();                         /* (LOAF BREAKER, 0.263: WIPE MEMORY, a sector a frame) */
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
    if (set.tut == TUT_ASK && !ui.lobo_t) {   /* LOAF BREAKER (0.229): the mode asked, after the start screen (0.236:
                                          * and after RESET ALL's own screen) */
        ask_draw();
        return;
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
        if (drop_kick_back && !playing_now()) {   /* (LOAF BREAKER, 0.211: st is filled further down: it was read
                                          * before it, the build's kick sometimes back at once) */
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
            if (set.tut == TUT_ASK) {    /* (LOAF BREAKER, 0.236, FLEXOR: RESET ALL as a first start: the screen state as
                                          * at power-on, the start screen, then the mode asked) */
                memset(&ui, 0, sizeof ui);
                keasy_pg = 0u;
                ask_sel = 1u;
                ask_drawn = 0xFFu;
                app_ui_init();
                return;
            }
        } else {
            ui.touched = 0u;
            return;
        }
    }
    ui.touched = 0u;
    ch_bta_poll(now);
    brk_status(&st);
    b = brk_get_break();
    {                                    /* LOAF BREAKER: AUTO SAVE. A change waits a minute, then is saved once PLAY
                                          * has been stopped 20 s (a flash write stalls the sound: never while playing;
                                          * 0.9.26, FLEXOR: 10 s after the sound's end came too soon; touching the FM-1
                                          * meanwhile holds nothing: "aucun problème tant que play est arrêté depuis 20
                                          * sec") */
#define AS_REST 20000u                   /* (AUTO SAVE: stopped this long first) */
        static uint32_t dirty_t, stop_t; /* (stop_t: when the break last played) */
        if (st.playing || !stop_t)
            stop_t = now | 1u;
        out_sound = 0u;
        if (!set_dirty || !set.auto_save || wiped || wipe_at)
            dirty_t = 0u;
        else if (!dirty_t)
            dirty_t = now | 1u;
        else if (now - dirty_t >= 60000u && !st.playing && now - stop_t >= AS_REST && !ui.lobo_t) {
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
        int want = build_p >= 0.0f && build_len && (b_style == DS_STUTTER || b_stut)
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
                                          * view (HOME's next part not taken); ~30 times a second (0.254) */
        int home = ui.page == PG_HOME || ui.page == PG_GEN || ui.page == PG_HSET;
        if (!(ui.btn & 1u << B_HOME))
            ui.cdj_skip = 0u;
        if (!ui.cdj_skip && home && ui.btn & 1u << B_HOME && now - ui.home_t >= 350u) {
            ui.blocks ^= 1u;
            set.bkeys.bk[BK_VIEW] = ui.blocks;   /* (0.9.2: HOME's SETTINGS' VIEW, kept) */
            set_dirty = 1u;
            ui.cdj_skip = 1u;
            ui.home_combo = 1u;
            ui.panel_drawn = 0u;
            ui.redraw |= RG_BLOCKS;
            app_say("HOME", ui.blocks ? "SLICES" : "WAVEFORM");
        }
        ui.cdj = (uint8_t)(home && ui.page != PG_HSET && !ui.blocks);
        if (ui.cdj && now - ui.panel_drawn >= 33u) {   /* (about 30 times a second: 0.178-0.253 20, FLEXOR 0.254: 30
                                          * again, smoother) */
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
    scope_on = (uint8_t)(ui.page == PG_HOME || ui.page == PG_GEN || ui.page == PG_KICK);   /* (LOAF BREAKER: the
                                          * scrolling wave's work in the render, only where it shows) */
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
        if (sh > shed_seen) {            /* (LOAF BREAKER, 0.226: said as it is: a slot bypassed, or none to) */
            uint32_t k, on = 0u;
            for (k = 0; k < BRK_FX_SLOTS; k++)
                on |= set.fx.slot[k].on;
            app_say("OVERLOAD", on ? "FX SLOT BYPASSED" : "CPU FULL: FEWER SOUNDS");
        }
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
            if (ui.shown == PAD_TAPESTOP)
                ui.redraw |= RG_WAVE;    /* (the platter gone: the wave back) */
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
    {                                    /* LOAF BREAKER (0.258): SEL held a moment (not a click) or locked: the foot
                                          * shows SEL + K1-K4; let go, the page's again */
        uint8_t sf = (uint8_t)(ui.sel_lock || (ui.sel_raw && now - ui.sel_t >= 300u));
        if (sf != ui.sel_foot) {
            ui.sel_foot = sf;
            ui.redraw |= RG_FOOT;
        }
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
    if (ui.page == PG_SEQ && st.playing && !ui.seq_manual && !ui.note_key && !SEQ_BRK && !(bk_seq() & 1u)) {   /* LOAF BREAKER: a lane of 32 or 64
                                          * steps: the keys show the bar playing (ALGORITHM, SEQ again: not, until
                                          * stopped) */
        const pat_t *p = &pats.p[set.pattern % NPAT];
        uint32_t sl = ui.seq_lane % NSEQL, n = lane_len(p, sl) / 16u, bar, want;
        bar = sl < SL_KICK ? (st.step >= 16 ? 1u : 0u) : st.bar64 >= 0 ? (uint32_t)(st.bar64 >> 4) % n : 0u;
        want = sl - SL_BAR[sl] + bar;
        if (n > 1u && want != sl && want < NSEQL && (want >= SL_KICK || brk_shown(want))) {   /* (0.234: not onto a
                                          * break row hidden) */
            ui.seq_lane = (uint8_t)want;
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        }
    }
    tut_frame(&st);                      /* LOAF BREAKER: the TUTORIAL: its step done, the next */
    if (ui.rst_done && !rst_cool()) {    /* LOAF BREAKER: RESET's 3 s at rest over: its picture as it was */
        ui.rst_done = 0u;
        if (RST_ON)
            ui.redraw |= RG_BLOCKS | RG_FOOT;
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
    if (ui.rst_ask && (now - ui.rst_ask >= RST_ASK_MS || !RST_ON)) {   /* (0.239: GLO not pressed in time, or the page
                                          * left: nothing reset) */
        ui.rst_ask = 0u;
        app_say("RESET", "CANCELLED");
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
        ui.shown != PAD_TAPESTOP &&      /* (0.9.30: VINYL BREAK's platter over it) */
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
        if (ui.page == PG_OVL)
            draw_ovl();
        else if (ui.page == PG_TRACK || RST_ON)
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
        } else if (ui.page == PG_HSET)
            draw_hset();                 /* (LOAF BREAKER, 0.9.2: HOME's SETTINGS) */
        else if (ui.cdj)
            draw_cdj(&st);
        else
            draw_blocks(&st, ui.keys);
    }
    if (ui.redraw & RG_FOOT)
        draw_foot(&st);
    ui.redraw = 0u;
}

/* the break manager (the site's breaks/ page; LOAF BREAKER 0.9.32, CHOPPAH 1.4's, PRD A-54h): the player's breaks over
 * USB SysEx, F0 7D 43 48 CMD DATA F7 (7D: non-commercial; "CH"), DATA 7-bit packed (each 7 bytes after a byte of their
 * top bits); CMD's bits 4-6 the zone (app.c CU_ZB: 0, 1), bits 0-3 the command. Answers: F0 7D 43 48 0x40|CMD CODE DATA
 * F7, CODE 0 done, 1 playing (stop first), 2 too big or out of place, 3 flash, 4 the bank as written doesn't check.
 * Commands: 1 INFO -> {version 2 u8, playing u8, the player's breaks u8, zones u8, then each zone {its breaks u8,
 * sectors u8, 0 u16, capacity u32, size u32}}; 2 BEGIN {size u32}: the zone's sectors erased, its breaks gone; 3 DATA
 * {offset u32 (16 on: the header comes last), bytes}; 4 COMMIT {the 16-byte header}: written last, the bank checked;
 * 5 READ {offset u32, count u32 (256 at most)} -> {offset u32, bytes}; 6 DELETE. Writes only while stopped (they stall
 * the sound). The scratch is the canvas (LOAF BREAKER: no demo) */
static uint32_t cu_size, cu_z = 0xFFu;   /* the bytes BEGIN made room for, in which zone */
static uint32_t pack7(const uint8_t *s, uint32_t n, uint8_t *d)
{
    uint32_t i, k = 0, j;
    for (i = 0; i < n; i += 7u) {
        uint32_t hi = k++;
        d[hi] = 0;
        for (j = 0; j < 7u && i + j < n; j++) {
            d[hi] |= (uint8_t)((s[i + j] >> 7) << j);
            d[k++] = s[i + j] & 0x7Fu;
        }
    }
    return k;
}
static uint32_t unpack7(const uint8_t *s, uint32_t n, uint8_t *d)
{
    uint32_t i, k = 0, j;
    for (i = 0; i < n; i += 8u)
        for (j = 1; j < 8u && i + j < n; j++)
            d[k++] = (uint8_t)(s[i + j] | ((s[i] >> (j - 1u)) & 1u) << 7);
    return k;
}
static void cu_reply(uint32_t cmd, uint32_t code, const uint8_t *d, uint32_t n)
{
    uint8_t *f = (uint8_t *)(void *)cv_px;   /* (LOAF BREAKER: no demo: the canvas, a frame drawn again after) */
    uint32_t k;
    lcd_sync();
    f[0] = 0xF0;
    f[1] = 0x7D;
    f[2] = 0x43;
    f[3] = 0x48;
    f[4] = (uint8_t)(0x40u | cmd);
    f[5] = (uint8_t)code;
    k = 6u + pack7(d, n, f + 6);
    f[k++] = 0xF7;
    (void)plat_sysex_send(f, k);
}
static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
int app_sysex(const uint8_t *p, uint32_t n)
{
    uint8_t *u = (uint8_t *)(void *)cv_px, a[16];   /* (the canvas, its last blit done: lcd_sync) */
    uint32_t cmd, len, off, i, z, code = 0u;
    if (n < 4u || p[0] != 0x7Du || p[1] != 0x43u || p[2] != 0x48u)
        return 0;
    cmd = p[3] & 15u;
    z = p[3] >> 4 & 7u;
    if (z >= CU_NZ)
        return 0;
    lcd_sync();
    ui.idle_t = plat_ms();
    ui.redraw |= RG_ALL;                 /* (the canvas taken: the screen drawn again) */
    len = unpack7(p + 4, n - 4u > 600u ? 600u : n - 4u, u);
    if (cmd == 1u || cmd == 5u) {        /* INFO, READ: any time */
        if (cmd == 1u) {
            uint32_t v[1u + 3u * CU_NZ], y;
            v[0] = 2u | (uint32_t)playing_now() << 8 | n_user << 16 | CU_NZ << 24;
            for (y = 0; y < CU_NZ; y++) {
                const bank_header_t *h = (const bank_header_t *)plat_data(cu_at[y]);
                v[1u + 3u * y] = n_uz[y] | CU_ZS[y] << 8;
                v[2u + 3u * y] = CU_CAP(y);
                v[3u + 3u * y] = n_uz[y] ? 16u + h->data_size : 0u;
            }
            cu_reply(p[3], 0u, (const uint8_t *)v, sizeof v);
            return 1;
        }
        off = rd32(u);
        len = rd32(u + 4);
        if (len > 256u || off > cu_cap[z] || len > cu_cap[z] - off) {
            cu_reply(p[3], 2u, 0, 0u);
            return 1;
        }
        for (i = 0; i < 4u; i++)
            a[i] = u[i];
        {                                /* {offset, bytes}: packed straight from the flash (a, then the bytes) */
            const uint8_t *src = plat_data(cu_at[z] + off);
            uint8_t *f = u;
            uint32_t k;
            f[0] = 0xF0; f[1] = 0x7D; f[2] = 0x43; f[3] = 0x48; f[4] = (uint8_t)(0x40u | p[3]); f[5] = 0u;
            k = 6u;
            for (i = 0; i < 4u + len; i += 7u) {   /* (the groups of 7 cross from the offset into the bytes) */
                uint8_t g[7];
                uint32_t j;
                for (j = 0; j < 7u; j++)
                    g[j] = i + j < 4u ? a[i + j] : i + j < 4u + len ? src[i + j - 4u] : 0u;
                k += pack7(g, 4u + len - i < 7u ? 4u + len - i : 7u, f + k);
            }
            f[k++] = 0xF7;
            (void)plat_sysex_send(f, k);
        }
        return 1;
    }
    if (playing_now()) {                 /* the writes stall the sound: stopped only */
        app_say("STOP FIRST", "TO LOAD A BREAK");
        cu_reply(p[3], 1u, 0, 0u);
        return 1;
    }
    if (cmd == 2u || cmd == 6u) {        /* BEGIN, DELETE: room made, the zone's breaks gone */
        uint32_t first = n_synth + n_bank + (z ? n_uz[0] : 0u);   /* (the zone's first break, and those after it) */
        cu_size = cmd == 2u ? rd32(u) : 1u;
        cu_z = z;
        if (cu_size < 16u + 168u && cmd == 2u)
            code = 2u;
        for (i = 0; !code && i < (cu_size + 4095u) / 4096u; i++)
            if (i >= CU_ZS[z] || plat_data_erase(CU_ZB[z] + i * 4096u))
                code = i >= CU_ZS[z] ? 2u : 3u;
        if (!code && !z && cu_at[0] == CU_OLD && plat_data_erase(CU_OLD))   /* (a 0.9.32 TEST's gone too) */
            code = 3u;
        if (code)
            cu_z = 0xFFu;
        if (set.brk >= first && set.brk < n_breaks)   /* (it was one of them, or after: the bank's first) */
            break_ask(n_synth);
        breaks_count();
        if (cmd == 2u && !code)
            app_say("RECEIVING", "A BREAK");
    } else if (cmd == 3u) {              /* DATA: after the header, into what BEGIN erased, a sector at a time (a
                                          * write stays in one window) */
        uint32_t at, k;
        off = rd32(u);
        len -= 4u;
        if (z != cu_z || len > 600u || off < 16u || off > cu_size || len > cu_size - off)
            code = 2u;
        for (i = 0; !code && i < len; i += k) {
            at = CU_ZB[z] + off + i;
            k = 4096u - (at & 0xFFFu);
            k = k < len - i ? k : len - i;
            if (plat_data_write(at, u + 4 + i, k))
                code = 3u;
        }
        if (!code && !cu_at_ok(CU_ZB[z] + off, len)) {   /* (an update record's mark: undone, refused) */
            plat_data_erase((CU_ZB[z] + off) & ~0xFFFu);
            plat_data_erase((CU_ZB[z] + off + len - 1u) & ~0xFFFu);
            code = 2u;
        }
        if (code)
            cu_z = 0xFFu;
    } else if (cmd == 4u) {              /* COMMIT: the header last, then the whole checked */
        if (z != cu_z || len < 16u || plat_data_write(CU_ZB[z], u, 16u))
            code = 3u;
        cu_z = 0xFFu;
        breaks_count();
        if (!code && !n_uz[z])
            code = 4u;
        break_ask(n_uz[z] ? n_synth + n_bank + (z ? n_uz[0] : 0u) : n_synth);   /* (LOAF BREAKER: the zone's first,
                                          * after the bank's; or the bank's first) */
    } else
        return 0;
    if (code > 1u)
        app_say("BREAK NOT LOADED", code == 4u ? "IT DOESN'T CHECK" : "FLASH OR SIZE");
    cu_reply(p[3], code, 0, 0u);
    return 1;
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
    if (!strcmp(cmd, "kset")) {          /* kset NAME VALUE: one of the kick's settings exactly as its knob shows it (the
                                          * listening pages of kicks FLEXOR makes presets from): disto, disto2 0..100;
                                          * dtype 0..8 (FOLD ..); drive, grit, punch, click, sub (levels) %; subdrv %
                                          * (0: the SUB's effect off); tdrive (TAIL +'s DRIVE) 0..100; tmode 0 CLASSIC,
                                          * 1 CLEAN; eqlow, eqmid, eqhigh dB; drop, dshape, hold -50..50; dtime MS */
        kick_t *k = &set.kick;
        float v = strtof(b, 0);
        int32_t n = atoi(b);
        if (!strcmp(a, "disto"))
            KICK_TERROR(k) = (uint8_t)clamp(n, 0, 100);
        else if (!strcmp(a, "disto2"))
            KICK_TERROR2(k) = (uint8_t)clamp(n, 0, 100);
        else if (!strcmp(a, "dtype"))
            KICK_T_TYPE(k) = (uint8_t)clamp(n, 0, (int32_t)KICK_T_TYPES - 1);
        else if (!strcmp(a, "drive"))
            k->intensity = v * 0.01f;
        else if (!strcmp(a, "sub") || !strcmp(a, "punch") || !strcmp(a, "grit") || !strcmp(a, "click"))
            k->l[!strcmp(a, "sub") ? KL_SUB : !strcmp(a, "punch") ? KL_PUNCH : !strcmp(a, "grit") ? KL_GRIT : KL_CLICK]
                .level = v * 0.01f;
        else if (!strcmp(a, "subdrv")) {
            kick_layer_t *s = &k->l[KL_SUB];
            s->fx = n > 0 ? KFX_DRIVE : KFX_OFF;
            s->fa = v * 0.01f;
            if (n > 0 && s->wet < 0.05f)
                s->wet = 1.0f;
        } else if (!strcmp(a, "tdrive"))
            klab.cur.drive = (uint8_t)clamp(n, 0, 100);
        else if (!strcmp(a, "tmode"))
            klab.cur.mode = (uint8_t)(n ? 1u : 0u);
        else if (!strcmp(a, "eqlow") || !strcmp(a, "eqmid") || !strcmp(a, "eqhigh"))
            set.keq[!strcmp(a, "eqlow") ? 0 : !strcmp(a, "eqmid") ? 1 : 2] = 0.5f + v / 24.0f;
        else if (!strcmp(a, "drop") || !strcmp(a, "dshape") || !strcmp(a, "hold"))
            set.ksyn[!strcmp(a, "drop") ? KS_DROP : !strcmp(a, "dshape") ? KS_SHAPE : KS_HOLD] = (int8_t)clamp(n, -50, 50);
        else if (!strcmp(a, "dtime")) {  /* (ks_text: 120 ms x 2^(v x 0.04)) */
            int32_t j, best = 0;
            for (j = -50; j <= 50; j++)
                if (brk_absf(120.0f * brk_exp2f_c((float)j * 0.04f) - v) <
                    brk_absf(120.0f * brk_exp2f_c((float)best * 0.04f) - v))
                    best = j;
            set.ksyn[KS_TIME] = (int8_t)best;
        } else {
            printf("  kset: no setting %s\n", a);
            host_fails++;
            return 1;
        }
        settings_apply();
        printf("  kick %s %s\n", a, b);
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
                                    "mb_solo", "mb_thr_low", "mb_slot2_thr_low", "trk_on", "trk_pos", "fx_p1", "kfx_fa", "kfx", "knotes", "comp", "knfn", "half_on", "intensity", "kpage", "t_type", "t_screech", "drop_style", "selk1", "selk3_fn", "khits", "trk_cur", "trk_it", "fx_type", "mb_kick", "mb_pre", "fill", "kbass1", "accel", "trk_from", "arp_fx", "arp_amt", "arp_more", "arp_thru", "ovl_rate", "kpreset", "aud_d", "kslot", "dirty", "brk_lvl", "kick_vol", "sfx_on", "sel_lock", "kick_dj", "csub", "kmorph", "kmorph_to", "ktune", "pbig", "playing", "kbar1", "kbar2", "kbar3", "selk1_fn", "selk4_fn", "kdrop", "ter_off", "terror", "mpage", "pgrp", "grp_dirty", "piano", "pcur", "hvn1", "hvn2", "hvn3", "minst", "mel_vol", "einst", "kpre", "kl_mode", "kl_note", "kl_src", "kl_end", "ab", "kview", "ksect", "wstyle", "kfocus", "rst_cool", "tut", "ezoom", "kv_ms", "beat", "lane_mask", "swing", "drop_bars", "ppre", "pfx", "pdirt", "proom", "lane_steps", "fx_scene", "keasy", "subdrv", "seq_brk", "brkview", "bk_pre", "bk_fn", "bk_key", "frz_len", "opage", "auto_save", "bhalf", "wiped", "skin", "bstut", "bgapm", "pads", "padlat", "fthru", "key", "key_n", "klen0", "rdiv0", "n_user", "n_breaks"};
    int v[136];
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
    v[14] = (int)(ovl_held | ovl_latched);
    v[15] = ovl_last;
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
    v[43] = 0;                           /* ("mb_kick": MB KICK, gone in 0.229) */
    v[44] = (int)mb_preset(fx_foc());
    v[45] = 0;                           /* ("fill": FILL, gone in 0.244) */
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
    v[80] = v[81] = v[82] = v[83] = v[84] = v[85] = v[86] = 0;   /* (KEYS and the MELODY's, 0.198-0.208: gone) */
    v[87] = ui.einst;
    v[88] = steps.kpre[0][8] + steps.kpre[1][8] + steps.kpre[2][8] + steps.kpre[3][8];
    v[89] = klab.cur.mode;
    v[90] = klab.cur.note;
    v[91] = klab.cur.src;
    v[92] = klab.cur.end;
    v[93] = ab_b;
    v[94] = ui.kclassic ? 2 : ui.kview;
    v[95] = ui.ksect;
    v[96] = set.wstyle;
    v[97] = set.kfocus;
    v[98] = rst_cool();
    v[99] = set.tut;
    v[100] = (int)ez_idx();
    v[101] = (int)kv_ms();
    v[102] = 0;                          /* ("beat": BEGINNER's BEAT, gone in 0.229) */
    v[103] = (int)sl_get(pat_now(), ui.seq_lane % NSEQL);
    v[104] = set.swing;
    v[105] = set.drop_bars;
    v[106] = perc_x(ui.pvoice)[PX_PRE];
    v[107] = perc_x(ui.pvoice)[PX_FX];
    v[108] = perc_x(ui.pvoice)[PX_DIRT];
    v[109] = perc_x(ui.pvoice)[PX_ROOM];
    v[110] = (int)(16u * sl_bars(&pats.p[set.pattern % NPAT], ui.seq_lane));
    v[111] = 0;                          /* ("fx_scene": the scenes, gone in 0.229) */
    v[112] = keasy_pg;
    v[113] = set.kick.l[KL_SUB].fx == KFX_DRIVE ? (int)(set.kick.l[KL_SUB].fa * 100.0f + 0.5f) : -1;
    v[114] = SEQ_BRK ? 1 : 0;
    v[115] = (int)brk_view();
    v[116] = (int)bk_preset();           /* (LOAF BREAKER: the black keys: the preset, the key BLACK KEYS edits and */
    v[117] = (int)bk_fn(ui.bk_sel);      /* its function, FREEZE's LENGTH, ARP's tab) */
    v[118] = ui.bk_sel;
    v[119] = set.pads.frz_len;
    v[120] = ui.opage;
    v[121] = set.auto_save;
    v[122] = set.bkeys.bk[BK_BHALF] & 1u;
    v[123] = wiped ? 2 : wipe_at ? 1 : 0;   /* (the host keeps the store apart: the erase itself is the FM-1's) */
    v[124] = (int32_t)skin_idx();
    v[125] = build_p >= 0.0f && b_stut;
    v[126] = build_p >= 0.0f ? b_gapm : 0;
    {
        brk_status_t ps;
        brk_status(&ps);
        v[127] = (int)ps.pads;
    }
    v[128] = pad_latched;
    v[129] = (int)bk_fthru();
    {
        brk_status_t ks;
        brk_status(&ks);
        v[130] = ks.key;
        v[131] = (int)ks.key_n;
    }
    v[132] = set.keys[0].len;
    v[133] = set.pads.roll_div[0];
    v[134] = (int)n_user;                /* (0.9.32: a custom break, the last) */
    v[135] = (int)n_breaks;
    for (i = 0; i < 136u; i++)
        if (!strcmp(what, W[i])) {
            *got = v[i];
            return 1;
        }
    return 0;
}
#endif
