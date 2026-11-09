/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the UI's state, its text helpers, messages, tap tempo, the screen's colours and big text. (Part of
 * app.c's unity: included there in order.) */

/* the screen's maths: the engine's shared copies (brk_math.c), not an inline copy at every call (the flash is full) */
#define brk_exp2f brk_exp2f_c
#define brk_log2f brk_log2f_c

/* LOAF BREAKER: WAVE ZOOM (GLO MORE, PRESETS): how much of the past the screen's width holds, in twelfths of a bar
 * (cdj.c; each pixel back is CDJ_Z / 12 of the render's columns): 1/2, 2/3, 5/6, 1 (as it was), 1 1/4, 1 1/2, 2 bars */
#define CDJ_ZN 7u
static const uint8_t CDJ_Z[CDJ_ZN] = {6u, 8u, 10u, 12u, 15u, 18u, 24u};
static const char *const CDJ_ZNAME[CDJ_ZN] = {"1/2 BAR", "2/3 BAR", "5/6 BAR", "1 BAR", "1 1/4 BARS", "1 1/2 BARS",
                                             "2 BARS"};
/* LOAF BREAKER: the screen's EFFECTS (GLO MORE K3), K1 their AMOUNT: every blit of the UI's goes through here and the
 * canvas (a strip of at most 240 x 60, big-endian RGB565) is worked on before it goes out. CRT: every other row dimmed;
 * VHS: bands that slip sideways, more on a kick; GRAIN: specks that move; RGB: the red a few pixels off the rest;
 * SHAKE: the strip pushed aside on a kick, hard at the drop; STROBE: the colours turned over at the drop (and on the
 * kicks, past half); 8-BIT: fewer colours. The kick and the drop are seen through kick_hit_n and build_p (app.c) */
enum { SFX_OFF, SFX_CRT, SFX_VHS, SFX_GRAIN, SFX_RGB, SFX_SHAKE, SFX_STROBE, SFX_8BIT, SFX_N };
static const char *const SFX_NAME[SFX_N] = {"OFF", "CRT", "VHS", "GRAIN", "RGB", "SHAKE", "STROBE", "8-BIT"};
static uint32_t sfx_rand(void)
{
    static uint32_t x = 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}
static void sfx_slide(uint16_t *p, uint32_t w, int32_t d)   /* a row moved d pixels (black where it left) */
{
    int32_t i, n = (int32_t)w;
    if (d > 0) {
        for (i = n - 1; i >= 0; i--)
            p[i] = i >= d ? p[i - d] : 0u;
    } else if (d < 0) {
        for (i = 0; i < n; i++)
            p[i] = i - d < n ? p[i - d] : 0u;
    }
}
static uint32_t sfx_kick_t, sfx_drop_t;   /* the last kick, the last drop (plat_ms | 1; 0 none) */
static uint8_t sfx_splash_d;            /* LOAF BREAKER: the splash's chromatic aberration (its red this far off; 0 none) */
static uint32_t sfx_win(uint32_t ms)     /* a kick's or a drop's moment, by GLO EFFECTS' LENGTH (50: as given) */
{
    return ms * (50u + (set.sfx_len > 100u ? 100u : set.sfx_len)) / 100u;
}
#define SFX_ON(e) (sfx_splash_d ? sfx_splash_d < 255u && (e) == SFX_RGB : (set.sfx_on >> ((e) - 1u)) & 1u)
                                         /* (several at once; sfx_splash_d 255: none, PRESS PLAY's wave) */
static int sfx_live(uint32_t now)       /* a kick's or a drop's moment of SHAKE, STROBE or VHS (main.c redraws the
                                          * whole screen as it starts and ends, so no strip keeps it) */
{
    static uint32_t kseen;
    static float bp_was = -1.0f;
    uint32_t sk, sd, km, dm;
    if (kick_hit_n != kseen) {
        kseen = kick_hit_n;
        sfx_kick_t = now | 1u;
    }
    if (bp_was >= 0.0f && build_p < 0.0f)
        sfx_drop_t = now | 1u;           /* (the build just ended: the drop) */
    bp_was = build_p;
    sk = sfx_kick_t ? now - sfx_kick_t : 99999u;
    sd = sfx_drop_t ? now - sfx_drop_t : 99999u;
    km = set.sfx_kick && sk < sfx_win(100u);
    dm = set.sfx_drop && sd < sfx_win(300u);
    return ((SFX_ON(SFX_SHAKE) || SFX_ON(SFX_VHS)) && (km || dm)) ||
           (SFX_ON(SFX_STROBE) && ((set.sfx_drop && sd < sfx_win(160u)) || (set.sfx_kick > 50u && km)));
}
static void crt_blit(uint32_t x, uint32_t y)   /* (the effects on, in this order: 8-BIT, GRAIN, CRT, RGB, VHS, SHAKE,
                                          * STROBE) */
{
    uint32_t amt = set.scrfx > 100u ? 100u : set.scrfx, now = plat_ms(), r, i;
    uint32_t sinc_kick = sfx_kick_t ? now - sfx_kick_t : 99999u, sinc_drop = sfx_drop_t ? now - sfx_drop_t : 99999u;
    uint32_t ks = set.sfx_kick > 100u ? 100u : set.sfx_kick, ds = set.sfx_drop > 100u ? 100u : set.sfx_drop;
    int km = ks && sinc_kick < sfx_win(100u), dm = ds && sinc_drop < sfx_win(300u);
    if (SFX_ON(SFX_8BIT)) {          /* 3 bits a colour, or 2 past half */
        uint16_t m = amt > 50u ? 0xC618u : 0xE71Cu;
        for (i = 0; i < cv_w * cv_h; i++)
            cv_px[i] = swap16((uint16_t)(swap16(cv_px[i]) & m));
    }
    if (SFX_ON(SFX_GRAIN)) {         /* specks: a pixel darkened or lit, amt / 8 % of them, more on a kick */
        uint32_t n = cv_w * cv_h * (amt + 4u) / 800u * (km ? 1u + ks / 50u : 1u);
        while (n--) {
            uint32_t at = sfx_rand() % (cv_w * cv_h);
            uint16_t c = swap16(cv_px[at]);
            cv_px[at] = swap16((sfx_rand() & 1u) ? (uint16_t)((c >> 1) & 0x7BEFu) : (uint16_t)(c | 0x2104u));
        }
    }
    if (SFX_ON(SFX_CRT)) {           /* the odd rows at 3/4, 1/2 or 1/4 */
        for (r = (y & 1u) ? 0u : 1u; r < cv_h; r += 2u) {
            uint16_t *p = cv_px + r * cv_w;
            for (i = 0; i < cv_w; i++) {
                uint16_t c = swap16(p[i]), h = (uint16_t)((c >> 1) & 0x7BEFu), q = (uint16_t)((c >> 2) & 0x39E7u);
                p[i] = swap16(amt < 34u ? (uint16_t)(h + q) : amt < 67u ? h : q);
            }
        }
    }
    if (SFX_ON(SFX_RGB)) {           /* the red 1..4 pixels right of the green and blue, more on a kick */
        uint32_t d = sfx_splash_d ? sfx_splash_d : 1u + amt / 34u + (km ? ks / 34u : 0u);
        for (r = 0; r < cv_h; r++) {
            uint16_t *p = cv_px + r * cv_w;
            for (i = cv_w; i-- > 0;) {
                uint16_t c = swap16(p[i]), red = i >= d ? (uint16_t)(swap16(p[i - d]) & 0xF800u) : 0u;
                p[i] = swap16((uint16_t)((c & 0x07FFu) | red));
            }
        }
    }
    if (SFX_ON(SFX_VHS)) {           /* a band or two slipping, more on a kick and at the drop */
        uint32_t bands = (km ? ks / 25u : 0u) + (dm ? 2u + ds / 20u : 0u) + (sfx_rand() % 100u < amt / 3u ? 1u : 0u);
        while (bands--) {
            uint32_t r0 = sfx_rand() % cv_h, n = 2u + sfx_rand() % (2u + amt / 10u);
            int32_t d = (int32_t)(1u + sfx_rand() % (1u + amt / 12u)) * ((sfx_rand() & 1u) ? 1 : -1);
            for (r = r0; r < cv_h && r < r0 + n; r++)
                sfx_slide(cv_px + r * cv_w, cv_w, d);
        }
    }
    if (SFX_ON(SFX_SHAKE) && (km || dm)) {   /* the strip pushed aside, hard at the drop */
        int32_t d = (int32_t)((1u + amt / 30u) * (dm ? 1u + ds / 25u : (ks + 49u) / 50u)) * ((now / 23u) & 1u ? 1 : -1);
        for (r = 0; r < cv_h; r++)
            sfx_slide(cv_px + r * cv_w, cv_w, d);
    }
    if (SFX_ON(SFX_STROBE) && ((ds && sinc_drop < sfx_win(160u)) || (ks > 50u && km)))
        for (i = 0; i < cv_w * cv_h; i++)
            cv_px[i] = (uint16_t)~cv_px[i];
    cv_blit(x, y);
}
#define cv_blit crt_blit

/* ---------------------------------------------------------------- skins --- */
/* GLOUPI BREAKS: the screen's colours, picked on GLOBAL's MORE page (K1) and saved (set.skin). fm1-base's palette
 * (gfx.c pal[]: rules, dim, labels, secondary, values) and the app's own (a block's ground, the block sounding, the
 * white, TAP TEMPO's unlit segments and metronome) from one table */
#define NSKINS 17u
static const char *const SKIN_NAME[NSKINS] = {"RED", "AMBER", "ICE BLUE", "PIP-BOY", "IRIS", "PINK", "MINT",
                                              "WHITE", "BLOOD", "MAXENCE", "SYNTHWAVE", "VAPOR",
                                              "TOXIC", "MIAMI", "LAVA", "GOLD", "BLAZE"};
/* each: line, dim, gray, mid (secondary), hi (values), block, white */
static const uint8_t SKIN_RGB[NSKINS][7][3] = {
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {232, 76, 60}, {255, 132, 112}, {24, 24, 30}, {255, 255, 255}},
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {232, 136, 30}, {255, 184, 70}, {24, 24, 30}, {255, 255, 255}},
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {60, 180, 235}, {150, 225, 255}, {24, 24, 30}, {255, 255, 255}},
    {{14, 44, 22}, {30, 100, 54}, {80, 180, 110}, {40, 220, 100}, {140, 255, 170}, {8, 28, 14}, {200, 255, 210}},
    {{34, 32, 44}, {76, 72, 90}, {152, 148, 166}, {150, 100, 240}, {196, 166, 255}, {24, 22, 34}, {255, 255, 255}},
    {{40, 32, 38}, {84, 72, 80}, {160, 148, 156}, {236, 72, 150}, {255, 146, 200}, {30, 22, 28}, {255, 255, 255}},
    {{30, 40, 38}, {68, 84, 80}, {146, 162, 158}, {40, 200, 160}, {130, 240, 205}, {20, 30, 28}, {255, 255, 255}},
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {196, 196, 202}, {236, 236, 240}, {24, 24, 30}, {255, 255, 255}},
    /* LOAF BREAKER's BLOOD: dried blood for the grounds, fresh blood for the values, bone for the words */
    {{46, 10, 12}, {100, 20, 24}, {168, 140, 128}, {160, 0, 12}, {230, 30, 36}, {26, 4, 6}, {232, 220, 196}},
    /* LOAF BREAKER's MAXENCE: a very loud pink, neon on plum */
    {{60, 12, 48}, {130, 36, 110}, {240, 170, 225}, {255, 0, 170}, {255, 70, 215}, {44, 0, 34}, {255, 232, 250}},
    /* LOAF BREAKER's SYNTHWAVE: a night sky's indigo, the values neon cyan, the secondary hot magenta, the words a
     * pale sunset pink */
    {{44, 22, 82}, {96, 58, 150}, {186, 160, 226}, {255, 46, 170}, {40, 236, 255}, {20, 8, 44}, {255, 214, 236}},
    /* LOAF BREAKER's two-colour skins: VAPOR (mint on lavender night, pink beside it), TOXIC (acid green, poison
     * purple), MIAMI (hot coral on deep teal, turquoise), LAVA (molten orange over crimson, on charcoal), GOLD (gold on
     * navy, royal blue) */
    {{50, 36, 70}, {110, 90, 140}, {200, 180, 220}, {255, 120, 200}, {120, 255, 230}, {28, 18, 44}, {240, 230, 255}},
    {{24, 40, 20}, {60, 90, 50}, {160, 190, 150}, {170, 60, 255}, {190, 255, 40}, {10, 18, 8}, {235, 255, 220}},
    {{20, 40, 56}, {50, 90, 110}, {150, 190, 200}, {0, 210, 200}, {255, 110, 150}, {8, 22, 32}, {255, 240, 230}},
    {{48, 24, 20}, {100, 56, 44}, {190, 160, 150}, {230, 30, 60}, {255, 140, 20}, {24, 10, 8}, {255, 240, 220}},
    {{24, 30, 60}, {60, 72, 120}, {160, 170, 210}, {70, 110, 255}, {255, 210, 90}, {10, 14, 34}, {240, 240, 255}},
    /* BLAZE: flashy orange values, pure red beside them, on a black with red in it */
    {{56, 12, 4}, {120, 30, 10}, {236, 156, 112}, {255, 16, 16}, {255, 112, 0}, {30, 4, 0}, {255, 238, 210}}};
static uint16_t skin_block = RGB(24, 24, 30), skin_hi = RGB(255, 132, 112), skin_white = 0xFFFFu;
static uint16_t skin_rgb(const uint8_t *c, uint32_t pct)   /* a colour at pct % of its brightness */
{
    return RGB(c[0] * pct / 100u, c[1] * pct / 100u, c[2] * pct / 100u);
}
static void skin_apply(uint32_t s)
{
    const uint8_t (*c)[3] = SKIN_RGB[s % NSKINS];
    uint32_t k;
    for (k = 0; k < 5u; k++)             /* fm1-base's five (C_LINE .. C_HI) */
        pal[k] = skin_rgb(c[k], 100u);
    skin_block = skin_rgb(c[5], 100u);
    skin_hi = skin_rgb(c[4], 100u);
    skin_white = skin_rgb(c[6], 100u);
}
#undef C_WHITE                           /* (the app's white is the skin's: put back after the UI, app.c's end) */
#define C_WHITE skin_white

enum { PG_HOME, PG_GEN, PG_FX, PG_GLOBAL, PG_KICK, PG_MIX, PG_SEQ, PG_PERC, PG_TRACK, PG_OVL, PG_KEQ, NPAGES };
                                         /* (LOAF BREAKER: PG_KEQ, HOME's third part, the KICK EQ; PG_TRACK, ENV's
                                          * TRACKS, PLAY, DROP where TAP TEMPO was) */
static const char *const PAD_NAME[BRK_NPADS] = {"R8", "R16", "R32", "REV", "SUB", "HLF", "VNL", "GAT", "RND",
                                                "FLT", "ARP"};
static const uint8_t WK[16] = PLAT_WHITE_KEYS, BK[11] = PLAT_BLACK_KEYS;
/* GLOUPI BREAKS: the black key (0..10, left to right) of each pad (PAD_*): DUB on the fifth key, FREEZE's in CHOPPAH,
 * and FREEZE on the last, DUB's. BK[PAD_KEY[pad]] is the pad's key; the strip of names follows the keys */
static const uint8_t PAD_KEY[BRK_NPADS] = {0, 1, 2, 3, 10, 5, 6, 7, 8, 4, 9};   /* (and FILTER on the fifth key,
                                          * DUB's; PAD_DUB the ARP key on the tenth, FILTER's: DUB itself gone) */

static struct {
    uint32_t keys, btn;
    uint8_t page, fx_focus;              /* fx_focus: the FX chain's slot the FX page tunes (0..3) */
    char msg[2][24];
    uint32_t msg_until;
    uint32_t tap_t;                      /* SEL went down then: a tap if it comes up alone */
    uint8_t tap_pend, tap_combo;         /* SEL down as a tap; something else touched while it was held */
    uint32_t oct_t, oct_rep;             /* OCT- / OCT+ held: repeats that speed up */
    int32_t last_step, last_slice;
    uint32_t last_pads, last_keys, last_hdr;
    uint32_t kick_t;                     /* LOAF BREAKER TEST: when the kick last hit (the header's flash) */
    uint32_t kick_seen;
    uint32_t kscope_drawn;
    uint32_t aud_t;                      /* LOAF BREAKER: a drum played on its key (the classic view, the drums' pages):
                                          * when, which */
    uint8_t aud_d;
    uint8_t pad_more;                    /* LOAF BREAKER: the ARP key's knobs: 0 MAIN, 1 MORE (FX while it is held) */
    uint8_t pbig;                        /* LOAF BREAKER: the drums' page in its big view (SELECT from the KICK menu's big
                                          * view); 0: the classic one (SEL + EDIT) */
    uint32_t intro_t0;                   /* the start screen: when it began (0: over) */
    uint8_t intro, intro_drawn;          /* showing it; blocks drawn so far */
    uint8_t touched;                     /* a key, button or encoder moved since app_frame last looked */
    uint32_t panel_drawn;                /* the GLOBAL panel's, the MB COMP meter's last frame */
    int8_t pad_focus;                    /* the pad held last (its animation, K1-K4 tune it); -1 none */
    int8_t key_focus;                    /* the white key held that loops (its view, K1-K4 tune it); -1 none */
    uint8_t key_layer;                   /* K1-K4 on a key held: 0 its loop, 1 its slice's effect (FX flips) */
    uint8_t key_turned;                  /* a knob turned while it is held: its view at once */
    uint32_t key_since;                  /* when it was pressed */
    int16_t shown;                       /* the strip shows: -1 the blocks, 0..10 a pad, 100 + k white key k */
    uint8_t gpage;                       /* the GLOBAL page's part: 0 MIDI and Bluetooth, 1 pitch, 2 BT audio */
    uint8_t bta_sel, bta_booted;         /* BT AUDIO: the device chosen in its list; the boot's reconnect done */
    uint8_t kpage, klayer, kpreset;      /* the KICK page: its sub-page (KP_*), the layer it edits, the preset */
    uint8_t mpage;                       /* the MIXER's half: 0 LEVELS (break, kick, perc, FX), 1 GROOVE */
    uint8_t seq_lane, seq_step;          /* the SEQ page: the lane (SL_*) and the break step (0..31) it edits */
    uint8_t pvoice, ppage, ppreset[PERC_VOICES];   /* the SNARE / HAT page: its voice, sub-page, presets */
    uint8_t einst;                       /* EDIT, the drums' design: DR_*: the kick (PG_KICK), the snare, the hat, the
                                          * clap (PG_PERC) */
    uint8_t rec;                         /* REC: white keys and MIDI notes write into the pattern as it plays */
    uint8_t help;                        /* SEL pressed alone: the SEL combinations on the strip, until the next press */
    uint32_t help_t;                     /* when the card opened */
    uint8_t drawn_page;                  /* the page last drawn (TAP takes the whole screen: leaving it redraws all) */
    uint32_t idle_t;                     /* the last key, button or knob touched (the demo: demo.c) */
    uint32_t lobo_t;                     /* RESET ALL done: the lobotomy screen, since then (0: not showing) */
    uint8_t home_hold, home_combo;       /* HOME held on its page (up alone: its other part); PLAY pressed meanwhile */
    uint8_t cdj, cdj_skip, blocks;       /* LOAF BREAKER: the CDJ view up (PERFORM / GENERATOR's default); this HOME
                                          * press done with it; the slice blocks instead (HOME held: either) */
    uint32_t home_t;                     /* (when HOME went down) */
    uint8_t fx_hold, fx_turned;          /* FX held on the FX page (up alone: the next slot); a knob turned meanwhile */
    uint8_t fx_page;                     /* the focused slot's part: MAIN, MORE, MOD (an MB COMP: MBP_*) */
    float mb_hold[3];                    /* its bands' reduction peaks held (dB), and since when */
    uint32_t mb_hold_t[3];
    uint32_t reset_t;                    /* GLOBAL's RESET ALL: when it was last turned */
    uint32_t drop_t, build_said;         /* LOAF BREAKER: the DROP's flash since (0: not); the build's last word */
    uint8_t drop_seen;                   /* the drops shown (drop_n's) */
    uint8_t drop_pat;                    /* the pattern the drop moves to (0xFF: none) */
    uint8_t drop_early, drop_asked;      /* (crossbreed) the drop asked early; its break asked for */
    uint8_t note_key, note_turned;
    uint8_t fx_open;
    uint8_t keq_part;                    /* (the KICK EQ page: 0 its bands, 1 their shape) */                     /* (FX pressed off its page: the page at its release, if not held) */       /* (crossbreed) SEQ, a kick lane: the key held + 1 (0 none); K1 turned since */
    uint8_t tpage;                       /* LOAF BREAKER: ENV's part: 0 TRACKS, 1 PLAY, 2 DROP, 3 KNOB */
    float radio;                         /* LOAF BREAKER: ENV PLAY's dial: where its needle is (eased to the track) */
    uint8_t ksect, kpos[4];
    uint8_t kclassic, edit_hold;         /* (EDIT held on the KICK menu: the view as it was, the drums' shapes; EDIT
                                          * down: when, and whether it has done its hold) */
    uint32_t edit_t;              /* LOAF BREAKER: the KICK menu's section (SOUND SYNTH DIRT TONE), each one's part */
    uint8_t env_hold;                    /* LOAF BREAKER: ENV down on TRACKS: 1 waiting (let go: the next part), 2 held
                                          * (a gap made) */
    uint32_t env_t;
    uint8_t rst_sel;                     /* LOAF BREAKER: ENV's RESET: what it resets (RST_*) */
    uint32_t rst_arm;                    /* (the last click of K2-K4: when; RST_TURNS clicks, each within 2 s, the reset) */
    uint8_t rst_n;                       /* (the clicks so far) */
    uint8_t lfo_hold;                    /* LOAF BREAKER: LFO down (1), a level turned with it (2): no page then */
    uint8_t sfx_cur;                     /* LOAF BREAKER: GLO EFFECTS: the effect K1 points at (SFX_CRT..SFX_8BIT) */
    uint8_t sel_lock, sel_raw;           /* LOAF BREAKER: SEL locked down (SEL x2; pressed again: free); SEL's
                                          * own state last frame */
    uint8_t knfn;                        /* KNOB's function: 0 DUCK (K1), 1 TERROR 2, 2 COMP, 3 TERROR (K4) */
    uint8_t fx_main, kfx_last;           /* (KICK FX: the FX page on slot 4 or 5; the chain's slot kept meanwhile, and
                                          * the kick's last) */
    uint8_t trk_cur, trk_from;           /* TRACKS' item edited; PLAY's first item */
    uint8_t trk_on;                      /* a track playing (2: starting, not yet heard) */
    uint8_t trk_pos, trk_loops, trk_cnt; /* its item, the loops it played, this loop counted */
    uint8_t trk_drop, trk_stop;          /* the item a drop of it goes to (0xFF none); the stop at the loop's end */
    int16_t trk_prev;                    /* the step seen last (a wrap: a new loop) */
    uint8_t sel_clicks;                  /* SEL's clicks in a row (three: the build) */
    uint32_t sel_click_t;
    uint8_t reset_n;                     /* GLOUPI BREAKS: its turns so far, each within 3 s of the last (it wants 15) */
    uint32_t mix_drawn;                  /* its faders' meters: when last drawn */
    int16_t meter_x;                     /* the header's level meter: where (0: not shown now, a message is) */
    uint32_t meter_drawn;
    uint32_t pad_since, fx_drawn;        /* when it was pressed; the animation's last frame */
    float reel;                          /* the tape stop's reels, turns */
    uint8_t redraw;                      /* RG_* bits: the regions to draw again */
} ui;
/* the white keys play slices (or, on the SEQ, KICK and SNARE / HAT pages, steps) */
static void page_say(void);              /* the header: the page's button and part, as its tabs (main.c) */
static int slices_page(void) { return ui.page != PG_SEQ && ui.page != PG_TRACK && ui.page != PG_OVL &&
                                         !(ui.page == PG_GLOBAL && ui.gpage == 3u); }   /* (GLO EFFECTS: keys 1-7) */   /* (ARP: overlays) */
enum { RG_HEAD = 1, RG_WAVE = 2, RG_BLOCKS = 4, RG_FOOT = 8, RG_ALL = 15, RG_FX = 16 };

static void text_cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static void int_text(int32_t v, char *b)
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void tenths_text(int32_t v, char *b)   /* 1700 -> "170.0" */
{
    char t[12];
    int_text(v / 10, b);
    t[0] = '.';
    t[1] = (char)('0' + (v < 0 ? -v : v) % 10);
    t[2] = 0;
    text_cat(b, t);
}

void app_say(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
    ui.redraw |= 1u;               /* RG_HEAD: the message */
}

static void say_int(const char *what, int32_t v, const char *unit)
{
    char t[24];
    int_text(v, t);
    text_cat(t, unit);
    app_say(what, t);
}

static float knob_f(float v, int32_t d, float step, float lo, float hi)
{
    v += (float)d * step;
    return v < lo ? lo : v > hi ? hi : v;
}

/* ------------------------------------------------------------- the FX chain --- */

static void hz_text(float hz, char *t)   /* 340 -> "340HZ", 1234 -> "1.2K" */
{
    t[0] = 0;
    if (hz >= 1000.0f) {
        tenths_text((int32_t)(hz / 100.0f + 0.5f), t);
        text_cat(t, "K");
    } else {
        int_text((int32_t)(hz + 0.5f), t);
        text_cat(t, "HZ");
    }
}

static void hundredths_text(float v, char *t)   /* 0.25 -> "0.25" (0..99.99) */
{
    int32_t c = (int32_t)(v * 100.0f + 0.5f);
    char d[4];
    int_text(c / 100, t);
    text_cat(t, ".");
    d[0] = (char)('0' + c / 10 % 10);
    d[1] = (char)('0' + c % 10);
    d[2] = 0;
    text_cat(t, d);
}

static void pct_text(float v, char *t)   /* 0.42 -> "42%" */
{
    int_text((int32_t)(v * 100.0f + (v < 0.0f ? -0.5f : 0.5f)), t);
    text_cat(t, "%");
}
static void signed_tenths(float v, const char *unit, char *t)   /* -1.5 -> "-1.5ST" */
{
    int32_t x = (int32_t)(v * 10.0f + (v < 0.0f ? -0.5f : 0.5f));
    t[0] = x > 0 ? '+' : 0;
    t[1] = 0;
    tenths_text(x, t + (x > 0));
    if (x < 0 && x > -10) {              /* tenths_text drops the sign of -0.x */
        t[0] = '-';
        tenths_text(-x, t + 1);
    }
    text_cat(t, unit);
}
static void times_text(float v, char *t) /* 0.5 -> "x0.50" */
{
    int32_t x = (int32_t)(v * 100.0f + 0.5f);
    char d[8];
    t[0] = 'x';
    int_text(x / 100, t + 1);
    text_cat(t, ".");
    d[0] = (char)('0' + x / 10 % 10);
    d[1] = (char)('0' + x % 10);
    d[2] = 0;
    text_cat(t, d);
}

/* ------------------------------------------------------------- start screen --- */
#define C_BLOCK skin_block              /* a slice block's ground (the skin's) */
#define C_BLOCK_HI skin_hi                /* the block sounding */

/* big text: a font at twice its size, each of its pixels a 2 x 2 block (blended as cv_text blends). It stands in for
 * a 31-pixel font: that one costs 20 KB of flash, which Bluetooth audio needed */
static int32_t text2_w(const felucca_font_t *f, const char *s) { return 2 * text_w(f, s); }
static int32_t cv_textn(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, uint32_t n);
static int32_t cv_text2(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    return cv_textn(x, y, f, s, c, 2u);
}
/* a font at n times its size (TAP's tempo: 3) */
static int32_t cv_textn(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, uint32_t n)
{
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
#ifdef FONT_TRIM                         /* (LOAF BREAKER: the fonts stored trimmed, fm1.toml font_trim: the glyph its lit
                                          * box, after its left, top and rows) */
        int32_t ox = gd[0], oy = gd[1];
        uint32_t rows = gd[2];
        gd += 3;
#else
        int32_t ox = 0, oy = 0;
        uint32_t rows = f->h;
#endif
        for (gy = 0; gy < rows; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t a = gd[gy * bpr + gx / 2u], k;
                a = (gx & 1u) ? (a & 15u) : (a >> 4);
                if (!a)
                    continue;
                for (k = 0; k < n * n; k++) {
                    int32_t px = x + (int32_t)n * ((int32_t)gx + ox - f->pad) + (int32_t)(k % n),
                            py = y + (int32_t)n * ((int32_t)gy + oy) + (int32_t)(k / n) + cv_oy;
                    uint32_t at, bg;
                    if ((uint32_t)px >= cv_w || py < cv_y0 || py >= cv_y1)
                        continue;
                    at = (uint32_t)py * cv_w + (uint32_t)px;
                    bg = swap16(cv_px[at]);
                    cv_px[at] = swap16((((r * a + (bg >> 11) * (15u - a)) / 15u) << 11) |
                                       (((g * a + ((bg >> 5) & 63u) * (15u - a)) / 15u) << 5) |
                                       ((b * a + (bg & 31u) * (15u - a)) / 15u));
                }
            }
        x += (int32_t)n * f->adv[gi];
    }
    return x;
}
