/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the GLOBAL page: MIDI clock, Bluetooth MIDI, pitch, snap, RESET ALL. (Part of app.c's unity: included
 * there in order.) */

static void reset_all(void);             /* (below, with the break's change) */
/* the GLOBAL page's knobs: clock in, clock out, Bluetooth, where MIDI goes */
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
static int tut_on(void);                 /* (LOAF BREAKER: the TUTORIAL, tutor.c: further on) */
static void tut_start(void);
static void tut_quit(void);
static void global_value(int i, char *t)
{
    t[0] = 0;
    if (ui.gpage == GP_FILL) {           /* (SEQ's SETTINGS: SWING; 0.9.31, FLEXOR: BREAK ROWS, FOLLOW) */
        if (i == 0) {
            int_text(set.swing, t);
            text_cat(t, "%");
        } else if (i == 1)
            text_cat(t, bk_seq() & 2u ? "16 STEPS" : "32 STEPS");
        else if (i == 2)
            text_cat(t, bk_seq() & 1u ? "OFF" : "ON");
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
        return;                          /* (K4 EDIT VIEW and PRESETS DRUM VIEW ZOOM until 0.9.28: KICK › KNOBS › TOOLS) */
    }
    if (ui.gpage == 2u) {                /* SETUP (FLEXOR, 0.213: the knobs in the screen's order, top to bottom): K1 SKIN,
                                          * K2 WAVE ZOOM, K3 MODE, K4 AUTO SAVE */
        if (i == 0)                      /* (LOAF BREAKER, 0.237: the names nine letters at most, to fit its column) */
            text_cat(t, SKIN_NAME[skin_idx()]);
        else if (i == 1)                 /* (WAVE ZOOM: HOME's SETTINGS since 0.9.2) */
            ;
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
                : "");                   /* (LOAF BREAKER: K3 the METER, EXPERT only; K4 BREAK TIME 0.256-0.9.1: HOME's
                                          * SETTINGS now) */
}

/* LOAF BREAKER (FLEXOR, 11 Nov 2026): BEGINNER's BREAK its LEVEL alone (K4; the pitch: OCT), GROOVE its SWING alone */
static int glo_knob_ok(int i)
{
    return !set.novice || ui.page != PG_GLOBAL || (ui.gpage == GP_FILL ? i < 3 : ui.gpage == GP_BREAK ? i == 3 : 1);
}
static void global_turn(int i, int32_t d)
{
    static const char *const NAME[4] = {"CLOCK IN", "CLOCK OUT", "BLUETOOTH", "BREAK TIME"};
    char t[16];
    if (!glo_knob_ok(i)) {
        easy_no();
        return;
    }
    if (ui.gpage == GP_FILL) {           /* LOAF BREAKER: SEQ's SETTINGS (GROOVE until 0.9.30): K1 SWING; 0.9.31, FLEXOR:
                                          * K2 BREAK ROWS, the FULL view's (right 32), K3 FOLLOW, the grid's bar the one
                                          * playing (right ON) */
        static const char *const SN[3] = {"SWING", "BREAK ROWS", "FOLLOW"};
        if (i == 0)
            set.swing = (uint8_t)clamp((int32_t)set.swing + d, 50, 75);
        else if (i < 3) {
            uint32_t b = i == 1 ? 2u : 1u;
            set.bkeys.bk[BK_SEQ] = (uint8_t)(0xD0u | (d > 0 ? bk_seq() & ~b : bk_seq() | b));
            if (brk_view())              /* (the FULL view's rows now) */
                set.brkv = (uint8_t)(BRKV_TAG | (bk_seq() & 2u ? 1u : 2u));
        } else
            return;
        set_dirty = 1u;
        global_value(i, t);
        app_say(SN[i], t);
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
                                         /* (K3 WAVE until 0.9.24: HOME's SETTINGS K2 alone; K4 EDIT VIEW until 0.9.28:
                                          * KICK › MORE › TOOLS, FLEXOR; neither named here since 0.9.30) */
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
            set.skin = (uint8_t)(SKIN_NEW | (skin_idx() + (d > 0 ? 1u : NSKINS - 1u)) % NSKINS);
            changed();                   /* (settings_apply: skin_apply) */
            app_say("SKIN", SKIN_NAME[skin_idx()]);
            ui.redraw = RG_ALL;
        } else if (i == 1) {             /* (K2 WAVE ZOOM until 0.9.1: HOME's SETTINGS, FLEXOR) */
            app_say("WAVE ZOOM", "HOME > SETTINGS");
            return;
        } else if (i == 2) {             /* K3 MODE: right EXPERT, left BEGINNER (left again, the TUTORIAL; right from
                                          * it, BEGINNER) */
            if (d < 0 && set.novice && !tut_on())
                tut_start();
            else if (d > 0 && tut_on()) {
                tut_quit();
                app_say("MODE", "BEGINNER");
            } else {
                set.novice = d > 0 ? 0u : 1u;
                if (set.novice) {
                    ui.meter_on = 0u;    /* (the METER: EXPERT's) */
                    ui.fx_page = 0u;     /* (the FX page: MAIN only) */
                    if (ui.fx_focus >= FX_EASY_SLOTS && ui.fx_focus < BRK_FX_SLOTS)
                        ui.fx_focus = 0u;   /* (0.229: the chain's two slots) */
                    if (ui.fx_main >= FX_EASY_SLOTS)
                        ui.fx_main = 0u;
                    novice_lengths();    /* (0.227, FLEXOR: the kick 32 steps, the drums 16; 0.246: all 16) */
                }
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
    } else {                             /* (K4 BREAK TIME, 0.256-0.9.1: HOME's SETTINGS now, FLEXOR) */
        app_say("BREAK TIME", "HOME > SETTINGS");
        return;
    }
    global_value(i, t);
    app_say(NAME[i], t);
    changed();
}

/* the GLOBAL page's panel: Bluetooth, the clock coming in, the clock going out, MIDI through each port */
/* LOAF BREAKER (0.9.30, FLEXOR): GLO EFFECTS' cards, two rows of four on rows 46-189, drawn into the strip from screen
 * row top: each effect's card a small picture (a block, a zigzag, a floor) with that effect done to it, its key and
 * name below, lit when on; the eighth what the keys do */
static void sfx_cards(int32_t top)
{
    uint32_t f, cur = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
    cv_oy = -top;
    for (f = SFX_CRT; f <= SFX_N; f++) {
        int32_t x = 4 + (int32_t)((f - 1u) % 4u) * 59, y = 46 + (int32_t)((f - 1u) / 4u) * 74, i, yy;
        char n[12] = {(char)('0' + f), ' ', 0};
        if (f == SFX_N) {                /* (the last place: what the keys do here) */
            cv_text(x + 2, y + 14, &FONT_XS, "KEYS 1-7:", C_DIM);
            cv_text(x + 2, y + 28, &FONT_XS, "ON / OFF", C_DIM);
            cv_text(x + 2, y + 42, &FONT_XS, "K1: PICK", C_DIM);
            break;
        }
        if (f == cur)
            cv_rect(x - 2, y - 2, 59, 74, C_WHITE);
        cv_rect(x, y, 55, 70, C_BLACK);
        cv_rect(x + 1, y + 1, 53, 50, C_LINE);       /* the picture: a sky, a floor, a block, a zigzag */
        cv_rect(x + 1, y + 38, 53, 13, C_DIM);
        cv_rect(x + 8, y + 12, 14, 14, C_HI);
        for (i = 0; i < 53; i++) {
            yy = (i * 3) % 24;
            cv_rect(x + 1 + i, y + 20 + (yy < 12 ? yy : 24 - yy), 1, 2, C_WHITE);
        }
        for (yy = y + 1; yy < y + 51; yy++) {   /* (the effect, on this strip's rows of it) */
            int32_t r = yy - top;
            uint16_t *p, tmp[53];
            if (r < 0 || r >= (int32_t)cv_h)
                continue;
            p = cv_px + (uint32_t)r * cv_w + (uint32_t)(x + 1);
            for (i = 52; i >= 0; i--) {
                uint32_t c = swap16(p[i]), h = ((uint32_t)i * 2654435761u) ^ ((uint32_t)yy * 40503u);
                h ^= h >> 13;
                if (f == SFX_CRT && (yy & 1))
                    c = (c >> 2) & 0x39E7u;
                else if (f == SFX_GRAIN && (h & 6u) == 0u)
                    c = h & 1u ? (c >> 1) & 0x7BEFu : c | 0x39E7u;
                else if (f == SFX_RGB)
                    c = (c & 0x07FFu) | (i >= 3 ? swap16(p[i - 3]) & 0xF800u : 0u);
                else if (f == SFX_SHAKE && i >= 5)
                    c = ((c >> 1) & 0x7BEFu) + ((swap16(p[i - 5]) >> 1) & 0x7BEFu);
                else if (f == SFX_STROBE)
                    c = ~c & 0xFFFFu;
                else if (f == SFX_8BIT)
                    c = swap16(p[i - i % 3]) & 0xC618u;
                tmp[i] = swap16(c);
            }
            if (f == SFX_VHS && ((yy - y) / 5) % 3 == 1) {   /* (a band in three slipped aside) */
                int32_t d = ((yy - y) / 15) & 1 ? 5 : -5;
                for (i = 0; i < 53; i++)
                    p[i] = tmp[(i - d + 53) % 53];
            } else
                for (i = 0; i < 53; i++)
                    p[i] = tmp[i];
            if (f == SFX_8BIT && (yy - y - 1) % 3 && r > 0)
                for (i = 0; i < 53; i++)
                    p[i] = p[i - (int32_t)cv_w];   /* (3 x 3 blocks, the row above's) */
        }
        cv_rect(x, y + 53, 55, 17, SFX_ON(f) ? C_HI : C_BLOCK);
        text_cat(n, SFX_NAME[f]);
        cv_text(x + 3, y + 55, text_w(&FONT_T, n) <= 50 ? &FONT_T : &FONT_XS, n, SFX_ON(f) ? C_BLACK : C_GRAY);
    }
    cv_oy = 0;
}
static void draw_global(const brk_status_t *st)
{
    static const char *const PORT_NAME[NPORTS] = {"USB", "BT", "TRS"};
    char t[32], v[16];
    uint32_t p;
    int bs = plat_ble_state();
    cv_begin(240, 44, C_BLACK);          /* LOAF BREAKER: no waveform on GLOBAL: its place the skins' on MORE, black
                                          * on the others */
    if (ui.gpage == GP_FILL) {           /* LOAF BREAKER: GROOVE: the swing (0.244: FILL gone), and the bar's 16ths
                                          * as it lays them out */
        static const char *const SN[3] = {"SWING", "BREAK ROWS", "FOLLOW"};
        uint32_t s;
        char v[12];
        cv_text(6, 1, &FONT_T, "SEQ SETTINGS", C_GRAY);   /* (0.9.31, FLEXOR: GROOVE until 0.9.30) */
        cv_text(6, 16, &FONT_XS, "K1 SWING  K2 THE BREAK'S ROWS IN FULL", C_DIM);
        cv_text(6, 28, &FONT_XS, "K3 FOLLOW: THE GRID ON THE BAR PLAYING", C_DIM);
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        for (s = 0; s < 3u; s++) {       /* (each setting's box, its value large) */
            global_value((int)s, v);
            cv_rect(4 + (int32_t)s * 78, 4, 74, 42, C_BLOCK);
            cv_text(8 + (int32_t)s * 78, 6, &FONT_XS, SN[s], C_GRAY);
            cv_text(8 + (int32_t)s * 78, 24, text_w(&FONT_B, v) <= 66 ? &FONT_B : &FONT_T, v, C_WHITE);
        }
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_XS, "THE BAR'S 16THS, SWUNG", C_DIM);
        for (s = 0; s < 16u; s++) {      /* (the odd 16ths pushed later as the swing grows) */
            int32_t x = 6 + (int32_t)s * 14 + ((s & 1u) ? (int32_t)(set.swing - 50u) * 6 / 25 : 0);
            cv_rect(x, 18, 10, 22, (s & 3u) ? C_BLOCK : C_HI);
        }
        cv_blit(0, 140);
        return;
    }
    if (ui.gpage == 3u) {                /* LOAF BREAKER: EFFECTS: the seven, lit when on (keys 1-7), K1's outlined;
                                          * 0.9.30, FLEXOR: bigger, each one's picture with its effect on it */
        static const uint8_t TOP[3] = {44u, 92u, 140u}, HT[3] = {48u, 48u, 52u};
        uint32_t b;
        for (b = 0; b < 3u; b++) {
            cv_begin(240, HT[b], C_BLACK);
            sfx_cards(TOP[b]);
            cv_blit(0, TOP[b]);
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
        cv_text(96, 1, &FONT_T, SKIN_NAME[skin_idx()], C_HI);
        for (s = 0; s < NSKINS; s++) {   /* a swatch each: its value colour on its ground, the one on outlined */
            int32_t x = 6 + (int32_t)(s % 12u) * 19, y = 18 + (int32_t)(s / 12u) * 13;   /* (LOAF BREAKER, 0.237:
                                          * two rows of twelve, FLEXOR's) */
            const uint8_t (*c)[3] = SKIN_RGB[s];
            if (s == skin_idx())
                cv_rect(x - 2, y - 2, 21, 15, C_WHITE);
            cv_rect(x, y, 17, 11, skin_rgb(c[5], 100u));
            cv_rect(x + 2, y + 2, 13, 4, skin_rgb(c[4], 100u));
            cv_rect(x + 2, y + 7, 13, 2, skin_rgb(c[3], 100u));
        }
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        cv_text(96, 26, &FONT_XS, "SCREEN EFFECTS: SELECT", C_DIM);   /* (WAVE ZOOM gone, 0.9.25: HOME's SETTINGS) */
        cv_text(6, 38, &FONT_T, "K3 MODE", C_GRAY);
        cv_text(96, 38, &FONT_T, tut_on() ? "TUTORIAL" : set.novice ? "BEGINNER" : "EXPERT", C_HI);
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "K4 AUTO SAVE", C_GRAY);
        cv_text(96, 2, &FONT_T, set.auto_save ? "ON" : "OFF", set.auto_save ? C_HI : C_DIM);
        cv_text(96, 16, &FONT_XS, "A MINUTE AFTER A CHANGE,", C_DIM);
        cv_text(96, 28, &FONT_XS, "ONCE STOPPED 20 S", C_DIM);
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
                                         /* (BREAK TIME gone, 0.9.25, FLEXOR: HOME's SETTINGS K4 alone) */
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
        text_cat(t, "TO USB");           /* (LOAF BREAKER, 0.259: no Bluetooth: USB, whatever MIDI TO says) */
        text_cat(t, st->playing ? "  SENDING" : "  (WHILE PLAYING)");
    }
    cv_text(96, 2, &FONT_T, t, set.clk_out && st->playing ? C_HI : set.clk_out ? C_WHITE : C_DIM);
    cv_text(6, 30, &FONT_T, "MIDI", C_GRAY);
    for (p = 0; p < NPORTS; p++) {       /* (LOAF BREAKER, 0.259: USB and TRS; no Bluetooth, no BT box) */
        int32_t x = 96 + (p == PORT_TRS ? 1 : 0) * 48;
        if (p != PORT_TRS && p != 0u)
            continue;
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

/* LOAF BREAKER (0.9.2, FLEXOR): HOME's SETTINGS, the waveform's settings where it is played: K1 VIEW (the waveform or
 * the slices; HOME held still flips it), K2 WAVE TYPE (REKORDBOX, SERATO; SYSTEM EFFECTS' K3 until 0.9.24), K3 WAVE ZOOM (SYSTEM
 * SETUP's K2 until 0.9.1), K4 BREAK TIME (SYSTEM MIDI's K4 until 0.9.1). VIEW saved in bkeys.bk[BK_VIEW] */
static const char *const HSET_NAME[4] = {"VIEW", "WAVE TYPE", "WAVE ZOOM", "BREAK TIME"};
static void hset_value(int i, char *t)
{
    t[0] = 0;
    text_cat(t, i == 0 ? (ui.blocks ? "SLICES" : "WAVEFORM") : i == 1 ? (set.wstyle ? "SERATO" : "REKORDBOX")
                : i == 2 ? CDJ_ZNAME[set.wzoom % CDJ_ZN] : (set.bkeys.bk[BK_BHALF] & 1u ? "HALF" : "NORMAL"));
}
static void hset_turn(int i, int32_t d)
{
    char t[16];
    if (i == 0) {
        ui.blocks = d > 0 ? 1u : 0u;     /* (right the slices, left the waveform) */
        set.bkeys.bk[BK_VIEW] = ui.blocks;
    } else if (i == 1)
        set.wstyle = d > 0 ? 1u : 0u;
    else if (i == 2)
        set.wzoom = (uint8_t)clamp((int32_t)(set.wzoom % CDJ_ZN) + (d > 0 ? 1 : -1), 0, CDJ_ZN - 1);
    else
        set.bkeys.bk[BK_BHALF] = d > 0 ? 1u : 0u;   /* (right HALF: a 115 break under a 230 kick at its own speed) */
    changed();
    hset_value(i, t);
    app_say(HSET_NAME[i & 3], t);
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
static void draw_hset(void)
{
    uint32_t i;
    char t[48], v[16];
    cv_begin(240, 52, C_BLACK);          /* the four, as cards: the name, the value large (K1-K4 left to right) */
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        cv_rect(x, 0, 56, 50, C_BLOCK);
        {                                /* (the name a word a line: the card's width) */
            const char *n = HSET_NAME[i];
            uint32_t j = 0u;
            while (n[j] && n[j] != ' ')
                j++;
            t[0] = 0;
            text_cat(t, n);
            t[j] = 0;
            cv_text(x + 4, 3, &FONT_XS, t, C_GRAY);
            if (n[j])
                cv_text(x + 4, 13, &FONT_XS, n + j + 1, C_GRAY);
        }
        hset_value((int)i, t);
        cv_text(x + 4, 32, text_w(&FONT_T, t) > 50 ? &FONT_XS : &FONT_T, t, C_HI);
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);          /* what they do */
    t[0] = 0;
    text_cat(t, set.bkeys.bk[BK_BHALF] & 1u ? "THE BREAK AT HALF: " : "THE BREAK AT THE TEMPO: ");
    tenths_text((int32_t)set.tempo_x10 / ((set.bkeys.bk[BK_BHALF] & 1u) ? 2 : 1), v);
    text_cat(t, v);
    text_cat(t, " BPM");
    cv_text(6, 2, &FONT_XS, "HOME HELD: WAVEFORM / SLICES", C_DIM);
    cv_text(6, 14, &FONT_XS, set.wstyle ? "WAVE COLOURS AS SERATO'S" : "WAVE COLOURS AS REKORDBOX'S", C_DIM);
    cv_text(6, 26, &FONT_XS, t, C_DIM);
    cv_text(6, 38, &FONT_XS, "SELECT: TEMPO", C_DIM);
    cv_blit(0, 140);
}
