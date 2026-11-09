/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the UI's state, its text helpers, messages, tap tempo, the screen's colours and big text. (Part of
 * app.c's unity: included there in order.) */

/* the screen's maths: the engine's shared copies (brk_math.c), not an inline copy at every call (the flash is full) */
#define brk_exp2f brk_exp2f_c
#define brk_log2f brk_log2f_c

/* LOAF BREAKER TEST: CRT (GLO MORE K3): every other row of the screen dimmed by half, as a tube's lines; every blit of
 * the UI's goes through here (the canvas's rows on the screen's odd rows halved, then out) */
static void crt_blit(uint32_t x, uint32_t y)
{
    if (set.crt) {
        uint32_t r, i;
        for (r = (y & 1u) ? 0u : 1u; r < cv_h; r += 2u) {
            uint16_t *p = cv_px + r * cv_w;
            for (i = 0; i < cv_w; i++) {
                uint16_t c = swap16(p[i]);
                p[i] = swap16((uint16_t)((c >> 1) & 0x7BEFu));
            }
        }
    }
    cv_blit(x, y);
}
#define cv_blit crt_blit

/* ---------------------------------------------------------------- skins --- */
/* GLOUPI BREAKS: the screen's colours, picked on GLOBAL's MORE page (K1) and saved (set.skin). fm1-base's palette
 * (gfx.c pal[]: rules, dim, labels, secondary, values) and the app's own (a block's ground, the block sounding, the
 * white, TAP TEMPO's unlit segments and metronome) from one table */
#define NSKINS 10u
static const char *const SKIN_NAME[NSKINS] = {"RED", "AMBER", "ICE BLUE", "PIP-BOY", "IRIS", "PINK", "MINT",
                                              "WHITE", "BLOOD", "MAXENCE"};
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
    {{60, 12, 48}, {130, 36, 110}, {240, 170, 225}, {255, 0, 170}, {255, 70, 215}, {44, 0, 34}, {255, 232, 250}}};
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
    uint32_t rst_arm;                    /* (K4 turned once: when; again within 3 s, the reset) */
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
static int slices_page(void) { return ui.page != PG_SEQ && ui.page != PG_TRACK && ui.page != PG_OVL; }   /* (ARP: overlays) */
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
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t a = gd[gy * bpr + gx / 2u], k;
                a = (gx & 1u) ? (a & 15u) : (a >> 4);
                if (!a)
                    continue;
                for (k = 0; k < n * n; k++) {
                    int32_t px = x + (int32_t)n * ((int32_t)gx - f->pad) + (int32_t)(k % n),
                            py = y + (int32_t)n * (int32_t)gy + (int32_t)(k / n) + cv_oy;
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
