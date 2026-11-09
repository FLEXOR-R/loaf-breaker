/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the GLOBAL page: MIDI clock, Bluetooth MIDI, pitch, snap, RESET ALL. (Part of app.c's unity: included
 * there in order.) */

static void reset_all(void);             /* (below, with the break's change) */
/* the GLOBAL page's knobs: clock in, clock out, Bluetooth, where MIDI goes */
static const char *const MIDI_TO_NAME[3] = {"USB+BT", "USB", "BT"};
static void global_value(int i, char *t)
{
    t[0] = 0;
    if (ui.gpage == 2u) {                /* GLOUPI BREAKS: MORE (in place of Bluetooth audio): K1 the skin */
        if (i == 3)                      /* (LOAF BREAKER: K4 the skin; SYNTHWAVE short, to fit its column) */
            text_cat(t, set.skin % NSKINS == 10u ? "SYNTHWV" : SKIN_NAME[set.skin % NSKINS]);
        else if (i == 1)                 /* LOAF BREAKER: K2 AUTO SAVE */
            text_cat(t, set.auto_save ? "ON" : "OFF");
        else if (i == 2)                 /* K3 EFFECTS: the screen's */
            text_cat(t, SFX_NAME[set.crt % SFX_N]);
        else                             /* K1 their AMOUNT */
            pct_text((float)set.scrfx * 0.01f, t);
        return;
    }
    if (ui.gpage) {                      /* the pitch: speed or keep length, the grains */
        if (i == 0)
            text_cat(t, pitch_keeps() ? "KEEP LEN" : "SPEED");
        else if (i == 1) {
            int_text(set.grain_ms ? set.grain_ms : 30, t);
            text_cat(t, " MS");
        } else if (i == 2)
            text_cat(t, SNAP_SHORT[snap_index()]);
        else
            if (ui.reset_t && plat_ms() - ui.reset_t < 3000u) {   /* GLOUPI BREAKS: 15 turns, the count so far */
                int_text((int32_t)ui.reset_n, t);
                text_cat(t, " / " CH_RESET_TEXT);
            } else
                text_cat(t, "TURN " CH_RESET_TEXT "X");
        return;
    }
    text_cat(t, i == 0 ? (set.clk_in ? "FOLLOW" : "OFF") : i == 1 ? (set.clk_out ? "ON" : "OFF")
                : "");                   /* (LOAF BREAKER: K3, K4 nothing: no Bluetooth, MIDI goes over USB) */
}

static void global_turn(int i, int32_t d)
{
    static const char *const NAME[4] = {"CLOCK IN", "CLOCK OUT", "BLUETOOTH", "MIDI TO"};
    char t[16];
    if (ui.gpage == 2u) {                /* MORE: K4 the skin, the whole screen in its colours at once */
        if (i == 3) {
            set.skin = (uint8_t)((set.skin + (d > 0 ? 1u : NSKINS - 1u)) % NSKINS);
            changed();                   /* (settings_apply: skin_apply) */
            app_say("SKIN", SKIN_NAME[set.skin % NSKINS]);
            ui.redraw = RG_ALL;
        } else if (i == 1) {             /* K2 AUTO SAVE: right on, left off */
            set.auto_save = d > 0 ? 1u : 0u;
            changed();
            app_say("AUTO SAVE", set.auto_save ? "ON" : "OFF");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else if (i == 2) {             /* K3 EFFECTS: OFF, CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT */
            set.crt = (uint8_t)clamp((int32_t)(set.crt % SFX_N) + (d > 0 ? 1 : -1), 0, SFX_N - 1);
            changed();
            app_say("EFFECTS", SFX_NAME[set.crt % SFX_N]);
            ui.redraw = RG_ALL;
        } else {                         /* K1 the effect's AMOUNT, 2 % a detent */
            char a[8];
            set.scrfx = (uint8_t)clamp((int32_t)set.scrfx + d * 2, 0, 100);
            changed();
            pct_text((float)set.scrfx * 0.01f, a);
            app_say("EFFECT AMOUNT", a);
            ui.redraw = RG_ALL;
        }
        return;
    }
    if (ui.gpage) {
        if (i == 0)
            set.pitch_keep = d > 0 ? 1u : 2u;   /* right KEEP LENGTH, left SPEED */
        else if (i == 1)
            set.grain_ms = (uint8_t)clamp((int32_t)(set.grain_ms ? set.grain_ms : 30u) + d * 5, 10, 80);
        else if (i == 2)
            set.snap = (uint8_t)(clamp((int32_t)snap_index() + (d > 0 ? 1 : -1), 0, SNAPS - 1) + 1);
        else {                           /* RESET ALL: GLOUPI BREAKS, 15 turns of K4, each within 3 s of the last
                                          * (CHOPPAH: 2); a pause starts the count again */
            char n[12];
            ui.reset_n = (uint8_t)(ui.reset_t && plat_ms() - ui.reset_t < 3000u ? ui.reset_n + 1u : 1u);
            ui.reset_t = plat_ms() | 1u;
            if (ui.reset_n >= CH_RESET_TURNS) {
                ui.reset_t = 0u;
                ui.reset_n = 0u;
                reset_all();
            } else {
                int_text((int32_t)ui.reset_n, n);
                text_cat(n, " / " CH_RESET_TEXT " TURNS");
                app_say("RESET ALL?", n);
            }
            foot_dirty = 1u;
            return;
        }
        global_value(i, t);
        app_say(i == 0 ? "PITCH" : i == 1 ? "GRAIN" : "SNAP", i == 0 && pitch_keeps() ? "KEEP LENGTH"
                : i == 2 ? SNAP_NAME[snap_index()] : t);
        changed();
        return;
    }
    if (i == 0) {
        set.clk_in = d > 0;
        if (!set.clk_in) {               /* the tempo is the knob's again */
            ext.i = ext.n = 0u;
            ext.run = 0u;
            ext.bpm = ext.used = 0.0f;
        }
    } else if (i == 1)
        set.clk_out = d > 0;
    else                                 /* (LOAF BREAKER: no Bluetooth: K3, K4 do nothing) */
        return;
    global_value(i, t);
    app_say(NAME[i], t);
    changed();
}

/* the GLOBAL page's panel: Bluetooth, the clock coming in, the clock going out, MIDI through each port */
static void draw_global(const brk_status_t *st)
{
    static const char *const PORT_NAME[NPORTS] = {"USB", "BT", "TRS"};
    char t[32], v[16];
    uint32_t p;
    int bs = plat_ble_state();
    if (ui.gpage == 2u) {                /* MORE: the skin, and every skin's colours to pick from */
        uint32_t s;
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "SKIN", C_GRAY);
        cv_text(96, 2, &FONT_T, SKIN_NAME[set.skin % NSKINS], C_HI);
        cv_text(96, 16, &FONT_XS, "K4: THE SCREEN'S COLOURS", C_DIM);
        for (s = 0; s < NSKINS; s++) {   /* a swatch each: its value colour on its ground, the one on outlined */
            int32_t x = 6 + (int32_t)(s % 9u) * 26, y = 27 + (int32_t)(s / 9u) * 13;   /* (LOAF BREAKER: 17, in two
                                          * rows of nine) */
            const uint8_t (*c)[3] = SKIN_RGB[s];
            if (s == set.skin % NSKINS)
                cv_rect(x - 2, y - 2, 27, 15, C_WHITE);
            cv_rect(x, y, 23, 11, skin_rgb(c[5], 100u));
            cv_rect(x + 3, y + 2, 17, 4, skin_rgb(c[4], 100u));
            cv_rect(x + 3, y + 7, 17, 2, skin_rgb(c[3], 100u));
        }
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "AUTO SAVE", C_GRAY);   /* LOAF BREAKER: K2 */
        cv_text(96, 2, &FONT_T, set.auto_save ? "ON" : "OFF", set.auto_save ? C_HI : C_DIM);
        cv_text(96, 16, &FONT_XS, "1 MIN. AFTER A CHANGE", C_DIM);
        cv_text(6, 30, &FONT_T, "EFFECTS", C_GRAY);   /* K3, K1 its amount */
        cv_text(96, 30, &FONT_T, SFX_NAME[set.crt % SFX_N], set.crt % SFX_N ? C_HI : C_DIM);
        if (set.crt % SFX_N) {
            char a[8];
            pct_text((float)set.scrfx * 0.01f, a);
            cv_text(170, 30, &FONT_T, a, C_WHITE);
        }
        cv_text(6, 43, &FONT_XS, "WAVE (PRESETS)", C_GRAY);   /* (cdj.c) */
        cv_text(96, 43, &FONT_XS, CDJ_ZNAME[set.wzoom % CDJ_ZN], C_HI);
        cv_blit(0, 140);
        return;
    }
    if (ui.gpage) {                      /* the pitch: what the mode does, the pitch now */
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "PITCH", C_GRAY);
        cv_text(96, 2, &FONT_T, pitch_keeps() ? "KEEP LENGTH" : "SPEED", C_HI);
        cv_text(96, 16, &FONT_XS, pitch_keeps() ? "SLICES KEEP LENGTH" : "PITCH CHANGES LENGTH", C_DIM);
        cv_text(6, 30, &FONT_T, "NOW", C_GRAY);
        t[0] = set.pitch > 0 ? '+' : 0;
        t[1] = 0;
        int_text(set.pitch, t + (set.pitch > 0));
        text_cat(t, " ST  (OCT- / OCT+)");
        cv_text(96, 30, &FONT_T, t, C_WHITE);
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "GRAIN", C_GRAY);
        int_text(set.grain_ms ? set.grain_ms : 30, t);
        text_cat(t, " MS");
        cv_text(96, 2, &FONT_T, t, pitch_keeps() ? C_WHITE : C_DIM);
        cv_text(96, 14, &FONT_XS, "LOWER: TIGHTER HITS", C_DIM);
        cv_text(6, 30, &FONT_T, "SNAP", C_GRAY);   /* where a key lands, and what that is in this break */
        t[0] = 0;
        text_cat(t, SNAP_NAME[snap_index()]);
        if (snap_index()) {
            const brk_break_t *b = brk_get_break();
            text_cat(t, " = ");
            sixteenths_text(SNAP_SL[snap_index()] * key_sps(b), v);
            text_cat(t, v);
        }
        cv_text(96, 30, &FONT_T, t, C_HI);
        cv_blit(0, 140);
        return;
    }
    cv_begin(240, 52, C_BLACK);          /* where MIDI goes (LOAF BREAKER: no Bluetooth), and the clock in */
    (void)bs;
    cv_text(6, 2, &FONT_T, "MIDI", C_GRAY);
    cv_text(96, 2, &FONT_T, "USB AND THE TRS JACK", C_WHITE);
    cv_text(6, 30, &FONT_T, "CLOCK IN", C_GRAY);
    t[0] = 0;
    if (!set.clk_in)
        text_cat(t, "OFF");
    else if (!ext_active())
        text_cat(t, "WAITING FOR CLOCK");
    else {
        tenths_text((int32_t)(ext.bpm * 10.0f + 0.5f), v);
        text_cat(t, v);
        text_cat(t, " BPM ");
        text_cat(t, PORT_NAME[ext.port % NPORTS]);
        text_cat(t, ext.run ? "  RUNNING" : "  STOPPED");
    }
    cv_text(96, 30, &FONT_T, t, ext_active() ? C_HI : set.clk_in ? C_WHITE : C_DIM);
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);          /* the clock out, and MIDI through each port (lit: in the last 0.3 s) */
    cv_text(6, 2, &FONT_T, "CLOCK OUT", C_GRAY);
    t[0] = 0;
    if (!set.clk_out)
        text_cat(t, "OFF");
    else {
        text_cat(t, "TO ");
        text_cat(t, MIDI_TO_NAME[set.midi_to % 3u]);
        text_cat(t, st->playing ? "  SENDING" : "  (WHILE PLAYING)");
    }
    cv_text(96, 2, &FONT_T, t, set.clk_out && st->playing ? C_HI : set.clk_out ? C_WHITE : C_DIM);
    cv_text(6, 30, &FONT_T, "MIDI", C_GRAY);
    for (p = 0; p < NPORTS; p++) {
        int32_t x = 96 + (int32_t)p * 48;
        int in = plat_midi_ago((int)p, 0) < 300u, out = p != PORT_TRS && plat_midi_ago((int)p, 1) < 300u;
        cv_rect(x, 28, 44, 18, C_BLOCK);
        cv_text(x + 4, 30, &FONT_T, PORT_NAME[p], in || out ? C_WHITE : C_DIM);
        cv_rect(x + 34, 31, 4, 4, in ? C_HI : C_LINE);       /* in */
        cv_rect(x + 34, 38, 4, 4, out ? C_HI : C_LINE);      /* out */
    }
    cv_blit(0, 140);
}

static void global_part(uint32_t g)      /* GLO (again: the next part) and ALGORITHM on GLOBAL */
{
    ui.gpage = (uint8_t)g;
    ui.page = PG_GLOBAL;
    ui.redraw |= RG_FOOT | RG_BLOCKS;
    page_say();
}
