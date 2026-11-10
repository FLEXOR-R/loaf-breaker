/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the GLOBAL page: MIDI clock, Bluetooth MIDI, pitch, snap, RESET ALL. (Part of app.c's unity: included
 * there in order.) */

static void reset_all(void);             /* (below, with the break's change) */
/* the GLOBAL page's knobs: clock in, clock out, Bluetooth, where MIDI goes */
static const char *const MIDI_TO_NAME[3] = {"USB+BT", "USB", "BT"};
static uint8_t *sfx_knob(int i)          /* LOAF BREAKER: GLO EFFECTS' K1-K4: AMOUNT, on a KICK, at the DROP, LENGTH */
{
    return i == 0 ? &set.scrfx : i == 1 ? &set.sfx_kick : i == 2 ? &set.sfx_drop : &set.sfx_len;
}
static void sfx_toggle(uint32_t e, int on)   /* LOAF BREAKER: GLO EFFECTS: effect e (SFX_*) on (1), off (0), or turned
                                          * over (-1) */
{
    uint8_t b = (uint8_t)(1u << (e - 1u));
    set.sfx_on = (uint8_t)(on < 0 ? set.sfx_on ^ b : on ? set.sfx_on | b : set.sfx_on & ~b);
    ui.sfx_cur = (uint8_t)e;
    changed();
    app_say(SFX_NAME[e], set.sfx_on & b ? "ON" : "OFF");
    ui.redraw = RG_ALL;
}
/* LOAF BREAKER (0.201, the pages by domain): two parts of this page's code are shown elsewhere: GP_BREAK, the break's
 * PITCH, GRAIN, SNAP and LEVEL, is SOUNDS' BREAK (SEL + EDIT); GP_FILL, SWING and FILL, is PATTERN's GROOVE (SEQ's
 * ALGORITHM past KEYS, SEL + ARP). SYSTEM (GLO) itself: MIDI, SETUP, EFFECTS, FX SET, RESET */
#define GP_BREAK 1u
#define GP_FILL 6u
#define NGLO 5u
static const uint8_t GLO_ORD[NGLO] = {0u, 2u, 3u, 4u, 5u};
static uint32_t glo_next(int32_t d)      /* SYSTEM's next or previous part (from the break's or GROOVE: MIDI) */
{
    uint32_t i;
    for (i = 0; i < NGLO; i++)
        if (GLO_ORD[i] == ui.gpage && ui.page == PG_GLOBAL)
            return GLO_ORD[(i + (d > 0 ? 1u : NGLO - 1u)) % NGLO];
    return 0u;
}
static const char *const FILL_DRUM_NAME[4] = {"SNARE", "CLAP", "HAT", "ALL 3"};
static const char *const FILL_LEN_NAME[7] = {"1 BEAT", "1 BEAT /8", "2 BEATS", "2 BEATS /8", "1 BAR", "1 BAR /8",
                                             "2 STEPS"};
static int tut_on(void);                 /* (LOAF BREAKER: the TUTORIAL, tutor.c: further on) */
static void tut_start(void);
static void tut_quit(void);
static uint32_t fill_len_i(void) { return FILL_LEN < 3u ? FILL_LEN * 2u + FILL_RATE : 6u; }   /* (LENGTH and RATE, one) */
static void global_value(int i, char *t)
{
    t[0] = 0;
    if (ui.gpage == GP_FILL) {           /* (GROOVE: SWING, FILL's LATCH, DRUM, LENGTH) */
        if (i == 0) {
            int_text(set.swing, t);
            text_cat(t, "%");
        } else
            text_cat(t, i == 1 ? (FILL_LATCH ? "ON" : "OFF") : i == 2 ? FILL_DRUM_NAME[FILL_DRUM]
                     : FILL_LEN_NAME[fill_len_i()]);
        return;
    }
    if (ui.gpage == 4u) {                /* (FX SET: each 0..100 %) */
        pct_text((float)*sfx_knob(i) * 0.01f, t);
        return;
    }
    if (ui.gpage == 3u) {                /* (EFFECTS: K1 the one pointed at, K2 it on or off) */
        uint32_t e = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
        if (i == 0)
            text_cat(t, SFX_NAME[e]);
        else if (i == 1)
            text_cat(t, SFX_ON(e) ? "ON" : "OFF");
        else if (i == 2)                 /* (LOAF BREAKER: K3 the waves' colours, K4 the KICK menu's picture) */
            text_cat(t, set.wstyle ? "SERATO" : "REKORDBOX");
        else
            text_cat(t, set.kfocus ? "PART" : "WHOLE");
        return;
    }
    if (ui.gpage == 2u) {                /* SETUP (FLEXOR, 0.213: the knobs in the screen's order, top to bottom): K1 SKIN,
                                          * K2 WAVE ZOOM, K3 MODE, K4 AUTO SAVE */
        if (i == 0)                      /* (SYNTHWAVE short, to fit its column) */
            text_cat(t, set.skin % NSKINS == 10u ? "SYNTHWV" : SKIN_NAME[set.skin % NSKINS]);
        else if (i == 1)
            text_cat(t, CDJ_ZNAME[set.wzoom % CDJ_ZN]);
        else if (i == 2)                 /* (TUTORIAL, left of BEGINNER) */
            text_cat(t, tut_on() ? "TUTORIAL" : set.novice ? "BEGINNER" : "EXPERT");
        else
            text_cat(t, set.auto_save ? "ON" : "OFF");
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
        else {                           /* (LOAF BREAKER: K4 the break's LEVEL, the MIXER's BREAK; RESET ALL: SYSTEM
                                          * RESET, EVERYTHING) */
            int_text(set.brk_level, t);
            text_cat(t, "%");
        }
        return;
    }
    text_cat(t, i == 0 ? (set.clk_in ? "FOLLOW" : "OFF") : i == 1 ? (set.clk_out ? "ON" : "OFF")
                : i == 2 && !set.novice ? (ui.meter_on ? "ON" : "OFF")
                : "");                   /* (LOAF BREAKER: K3 the METER, EXPERT only; K4 nothing: no Bluetooth) */
}

static void global_turn(int i, int32_t d)
{
    static const char *const NAME[4] = {"CLOCK IN", "CLOCK OUT", "BLUETOOTH", "MIDI TO"};
    char t[16];
    if (ui.gpage == GP_FILL) {           /* LOAF BREAKER: GROOVE: K1 SWING; FILL: K2 LATCH, K3 the drum, K4 how long (16ths
                                          * or 8ths) */
        static const char *const FN[4] = {"SWING", "FILL LATCH", "FILL DRUM", "FILL LENGTH"};
        uint32_t c = set.fill_cfg;
        if (i == 0)
            set.swing = (uint8_t)clamp((int32_t)set.swing + d, 50, 75);
        else if (i == 1)
            c = (c & ~1u) | (d > 0 ? 1u : 0u);
        else if (i == 2)
            c = (c & ~6u) | (uint32_t)clamp((int32_t)FILL_DRUM + (d > 0 ? 1 : -1), 0, 3) << 1;
        else {
            uint32_t l = (uint32_t)clamp((int32_t)fill_len_i() + (d > 0 ? 1 : -1), 0, 6);
            c = (c & ~56u) | (l < 6u ? (l / 2u) << 3 | (l & 1u) << 5 : 3u << 3);
        }
        set.fill_cfg = (uint8_t)c;
        if (!FILL_LATCH && !(ui.btn & 1u << B_ARP))
            fill_on = 0u;                /* (LATCH off: no FILL left on) */
        set_dirty = 1u;
        global_value(i, t);
        app_say(FN[i & 3], t);
        ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
        return;
    }
    if (ui.gpage == 3u) {                /* LOAF BREAKER: EFFECTS: K1 the effect, K2 it on (right) or off (left) */
        uint32_t e = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
        if (i == 0) {
            ui.sfx_cur = (uint8_t)clamp((int32_t)e + (d > 0 ? 1 : -1), SFX_CRT, SFX_N - 1);
            app_say("EFFECT", SFX_NAME[ui.sfx_cur]);
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else if (i == 1)
            sfx_toggle(e, d > 0);
        else if (i == 2) {               /* LOAF BREAKER: K3 WAVE, the waves' colours (the KICK menu, HOME's CDJ view):
                                          * left REKORDBOX, right SERATO */
            set.wstyle = d > 0 ? 1u : 0u;
            changed();
            app_say("WAVE COLOURS", set.wstyle ? "SERATO" : "REKORDBOX");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else {                         /* K4 KICK VIEW: the KICK menu's picture, left the WHOLE kick, right the PART
                                          * edited */
            set.kfocus = d > 0 ? 1u : 0u;
            changed();
            app_say("KICK VIEW", set.kfocus ? "THE PART EDITED" : "THE WHOLE KICK");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        }
        return;
    }
    if (ui.gpage == 4u) {                /* LOAF BREAKER: FX SET: K1 AMOUNT, K2 KICK, K3 DROP, K4 LENGTH, 5 % a detent */
        static const char *const SN[4] = {"EFFECT AMOUNT", "ON A KICK", "AT THE DROP", "MOMENTS' LENGTH"};
        uint8_t *p = sfx_knob(i);
        *p = (uint8_t)clamp((int32_t)*p + d * 5, 0, 100);
        changed();
        global_value(i, t);
        app_say(SN[i & 3], t);
        ui.redraw = RG_ALL;
        return;
    }
    if (ui.gpage == 2u) {                /* SETUP: K1 the skin, the whole screen in its colours at once */
        if (i == 0) {
            set.skin = (uint8_t)((set.skin + (d > 0 ? 1u : NSKINS - 1u)) % NSKINS);
            changed();                   /* (settings_apply: skin_apply) */
            app_say("SKIN", SKIN_NAME[set.skin % NSKINS]);
            ui.redraw = RG_ALL;
        } else if (i == 1) {             /* K2 WAVE ZOOM: HOME's wave, how much of the past it holds */
            set.wzoom = (uint8_t)clamp((int32_t)(set.wzoom % CDJ_ZN) + (d > 0 ? 1 : -1), 0, CDJ_ZN - 1);
            changed();
            app_say("WAVE ZOOM", CDJ_ZNAME[set.wzoom % CDJ_ZN]);
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else if (i == 2) {             /* K3 MODE: right EXPERT, left BEGINNER (left again, the TUTORIAL; right from
                                          * it, BEGINNER) */
            if (d < 0 && set.novice && !tut_on())
                tut_start();
            else if (d > 0 && tut_on()) {
                tut_quit();
                app_say("MODE", "BEGINNER");
            } else {
                set.novice = d > 0 ? 0u : 1u;
                if (set.novice)
                    ui.meter_on = 0u;    /* (the METER: EXPERT's) */
                changed();
                app_say("MODE", set.novice ? "BEGINNER" : "EXPERT");
            }
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else {                         /* K4 AUTO SAVE: right on, left off */
            set.auto_save = d > 0 ? 1u : 0u;
            changed();
            app_say("AUTO SAVE", set.auto_save ? "ON" : "OFF");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
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
        else                             /* (LOAF BREAKER: K4 the break's LEVEL) */
            set.brk_level = (uint8_t)clamp((int32_t)set.brk_level + d * 2, 0, 100);
        global_value(i, t);
        app_say(i == 0 ? "PITCH" : i == 1 ? "GRAIN" : i == 2 ? "SNAP" : "BREAK LEVEL", i == 0 && pitch_keeps()
                ? "KEEP LENGTH" : i == 2 ? SNAP_NAME[snap_index()] : t);
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
    else if (i == 2 && !set.novice) {    /* LOAF BREAKER: K3 the METER, EXPERT only (0.213; SETUP's before): right on,
                                          * left off (not saved): CPU, IMG, FR in the header */
        ui.meter_on = d > 0 ? 1u : 0u;
        app_say("METER", ui.meter_on ? "ON: CPU, IMG, FR" : "OFF");
        ui.redraw |= RG_HEAD | RG_FOOT;
        return;
    } else                               /* (LOAF BREAKER: no Bluetooth: K4 does nothing) */
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
    cv_begin(240, 44, C_BLACK);          /* LOAF BREAKER: no waveform on GLOBAL: its place the skins' on MORE, black
                                          * on the others */
    if (ui.gpage == GP_FILL) {           /* LOAF BREAKER: GROOVE: the swing, what ARP's FILL does, as cards */
        static const char *const L[4] = {"SWING", "FILL LATCH", "FILL DRUM", "FILL LEN"};
        uint32_t j;
        cv_text(6, 1, &FONT_T, fill_on ? "GROOVE  FILL ON" : "GROOVE", fill_on ? C_HI : C_GRAY);
        cv_text(6, 16, &FONT_XS, FILL_LATCH ? "ARP: ON, ARP AGAIN: OFF" : "ARP HELD: THE FILL", C_DIM);
        cv_text(6, 28, &FONT_XS, "STEPS SET FILL PLAY, NOT FILL REST", C_DIM);
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        for (j = 0; j < 4u; j++) {
            int32_t x = 4 + (int32_t)j * 59;
            char v[12];
            global_value((int)j, v);
            cv_rect(x, 4, 53, 42, j == 1 && FILL_LATCH ? C_HI : C_BLOCK);
            cv_text(x + 4, 6, &FONT_XS, L[j], j == 1 && FILL_LATCH ? C_BLACK : C_GRAY);
            cv_text(x + 4, 26, text_w(&FONT_T, v) <= 47 ? &FONT_T : &FONT_XS, v, j == 1 && FILL_LATCH ? C_BLACK : C_WHITE);
        }
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        {                                /* the bar: its 16ths, the ones the fill rolls lit */
            uint32_t s;
            cv_text(6, 2, &FONT_XS, "THE BAR: THE FILL'S 16THS", C_DIM);
            for (s = 0; s < 16u; s++) {
                int on = s >= FILL_FROM[FILL_LEN] && (!FILL_RATE || !(s & 1u));
                cv_rect(6 + (int32_t)s * 14, 18, 12, 22, on ? (fill_on ? C_WHITE : C_HI) : (s & 3u) ? C_BLOCK : C_LINE);
            }
        }
        cv_blit(0, 140);
        return;
    }
    if (ui.gpage == 3u) {                /* LOAF BREAKER: EFFECTS: the seven, lit when on (keys 1-7), K1's outlined */
        uint32_t e, cur = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
        cv_text(6, 2, &FONT_XS, "EDIT ZOOM", C_GRAY);   /* (LOAF BREAKER, 0.216: EDIT's pictures' time, PRESETS) */
        {
            int32_t x = cv_text(84, 1, &FONT_T, EZ_NAME[set.ezoom % EZ_N], C_HI);
            cv_text(x + 8, 2, &FONT_XS, "PRESETS", C_DIM);
        }
        cv_text(6, 16, &FONT_XS, "K3 WAVE", C_GRAY);   /* (LOAF BREAKER: the waves' colours, the kick's picture) */
        cv_text(84, 15, &FONT_T, set.wstyle ? "SERATO" : "REKORDBOX", C_HI);
        cv_text(6, 30, &FONT_XS, "K4 KICK VIEW", C_GRAY);
        cv_text(84, 29, &FONT_T, set.kfocus ? "THE PART EDITED" : "THE WHOLE KICK", C_HI);
        cv_blit(0, 44);
        for (e = 0; e < 2u; e++) {
            uint32_t j;
            cv_begin(240, 52, C_BLACK);
            for (j = 0; j < 4u; j++) {
                uint32_t f = SFX_CRT + e * 4u + j;
                int32_t x = 4 + (int32_t)j * 59;
                char n[2] = {(char)('0' + f), 0};
                if (f >= SFX_N) {        /* (the last place: what the keys do here) */
                    cv_text(x + 2, 8, &FONT_XS, "KEYS 1-7:", C_DIM);
                    cv_text(x + 2, 20, &FONT_XS, "ON / OFF", C_DIM);
                    cv_text(x + 2, 32, &FONT_XS, "SET: GLO", C_DIM);
                    break;
                }
                if (f == cur)
                    cv_rect(x - 2, 2, 57, 46, C_WHITE);
                cv_rect(x, 4, 53, 42, SFX_ON(f) ? C_HI : C_BLOCK);
                cv_text(x + 4, 6, &FONT_T, n, SFX_ON(f) ? C_BLACK : C_DIM);
                cv_text(x + 4, 26, &FONT_T, SFX_NAME[f], SFX_ON(f) ? C_BLACK : C_GRAY);
            }
            cv_blit(0, 88u + e * 52u);
        }
        return;
    }
    if (ui.gpage == 4u) {                /* LOAF BREAKER: FX SET: the effects' four settings as bars */
        static const char *const L[4] = {"AMOUNT", "ON A KICK", "AT THE DROP", "LENGTH"};
        uint32_t k;
        cv_text(6, 1, &FONT_T, "EFFECTS SET", C_GRAY);
        cv_text(96, 16, &FONT_XS, "FOR ALL THE EFFECTS ON", C_DIM);
        cv_text(96, 28, &FONT_XS, "KICK, DROP: 0 = NONE", C_DIM);
        cv_blit(0, 44);
        for (k = 0; k < 2u; k++) {
            uint32_t j;
            cv_begin(240, 52, C_BLACK);
            for (j = 0; j < 2u; j++) {
                uint32_t n = k * 2u + j, v = *sfx_knob((int)n);
                char a[8];
                int32_t y = (int32_t)j * 26;
                cv_text(6, y + 2, &FONT_T, L[n], C_GRAY);
                pct_text((float)v * 0.01f, a);
                cv_text(196, y + 2, &FONT_T, a, C_WHITE);
                cv_rect(96, y + 7, 94, 5, C_LINE);
                cv_rect(96, y + 7, (int32_t)(v * 94u / 100u), 5, C_HI);
            }
            cv_blit(0, 88u + k * 52u);
        }
        return;
    }
    if (ui.gpage == 2u) {                /* MORE: the skin, and every skin's colours to pick from (K4) */
        uint32_t s;
        cv_text(6, 1, &FONT_T, "K1 SKIN", C_GRAY);
        cv_text(96, 1, &FONT_T, SKIN_NAME[set.skin % NSKINS], C_HI);
        for (s = 0; s < NSKINS; s++) {   /* a swatch each: its value colour on its ground, the one on outlined */
            int32_t x = 6 + (int32_t)(s % 9u) * 26, y = 18 + (int32_t)(s / 9u) * 13;   /* (18, two rows of nine) */
            const uint8_t (*c)[3] = SKIN_RGB[s];
            if (s == set.skin % NSKINS)
                cv_rect(x - 2, y - 2, 27, 15, C_WHITE);
            cv_rect(x, y, 23, 11, skin_rgb(c[5], 100u));
            cv_rect(x + 3, y + 2, 17, 4, skin_rgb(c[4], 100u));
            cv_rect(x + 3, y + 7, 17, 2, skin_rgb(c[3], 100u));
        }
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "K2 WAVE ZOOM", C_GRAY);   /* (cdj.c; PRESETS too) */
        cv_text(96, 2, &FONT_T, CDJ_ZNAME[set.wzoom % CDJ_ZN], C_HI);
        cv_text(96, 16, &FONT_XS, "HOME'S WAVE, HOW WIDE", C_DIM);
        cv_text(96, 26, &FONT_XS, "SCREEN EFFECTS: GLO AGAIN", C_DIM);
        cv_text(6, 38, &FONT_T, "K3 MODE", C_GRAY);
        cv_text(96, 38, &FONT_T, tut_on() ? "TUTORIAL" : set.novice ? "BEGINNER" : "EXPERT", C_HI);
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "K4 AUTO SAVE", C_GRAY);
        cv_text(96, 2, &FONT_T, set.auto_save ? "ON" : "OFF", set.auto_save ? C_HI : C_DIM);
        cv_text(96, 16, &FONT_XS, "A MINUTE AFTER A CHANGE,", C_DIM);
        cv_text(96, 28, &FONT_XS, "AFTER 10 S OF SILENCE", C_DIM);
        cv_blit(0, 140);
        return;
    }
    cv_blit(0, 44);
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

static void global_part(uint32_t g)      /* GLO (again: the next part) and ALGORITHM on GLOBAL (LOAF BREAKER: and the
                                          * break's page, GROOVE) */
{
    ui.gpage = (uint8_t)g;
    ui.page = PG_GLOBAL;
    ui.redraw |= RG_FOOT | RG_BLOCKS;
    page_say();
}
