/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the black-key pads: their own settings (K1-K4 while held) and their animations. (Part of app.c's unity:
 * included there in order.) */

/* -------------------------------------------------- the pads' own settings --- */
static void ovl_push(void);              /* (ovl.c) */
static const char *const THRU_NAME[4] = {"BOTH", "BREAK", "KICK", "BOTH"};   /* the ARP key's THRU (ovl_pad 6-7) */
static uint8_t ovlpad_held, ovlpad_lat;  /* (the ARP key's: below) */
static int ovl_pad_on(void);
static uint8_t half_lat;                 /* LOAF BREAKER: HALF SPEED latched on (its LATCH: K3) */
/* what K1-K4 tune while a pad is held, by pad (brk.h PAD_*); 0: that knob does nothing for it */
static const char *const PAD_KNOB[BRK_NPADS][4] = {
    {"RATE", "FILTER", "DRUM FLT", "DECAY"}, {"RATE", "FILTER", "DRUM FLT", "DECAY"}, {"RATE", "FILTER", "DRUM FLT", "DECAY"},
    {"SPEED", 0, 0, 0}, {"LENGTH", "PITCH", 0, 0}, {"SPEED", "M. TEMPO", "LATCH", 0}, {"STOP", "SPIN UP", "THRU", 0},
    {"RATE", "LENGTH", 0, 0}, {"AMOUNT", "PITCH", 0, 0}, {"TARGET", "RESO", "SPEED", 0},
    {"EFFECT", "THRU", "ON / OFF", "LATCH"}};   /* (LOAF BREAKER: held only; K2 what goes through the ARP
                                          * effects: the kick, the break, or both) */   /* (GLOUPI BREAKS: PAD_DUB is the ARP key) */
static const char *const PAD_TITLE[BRK_NPADS] = {"ROLL", "ROLL", "ROLL", "REVERSE", "SUB DRIVE", "HALF SPEED",
                                                  "VINYL BREAK", "GATE", "RANDOM", "FILTER", "OVERLAY"};

/* -1..1 as the DJ filter's frequency: "LP 340HZ" / "HP 1.2K" */
static void dj_text(float dj, char *t)
{
    t[0] = 0;
    text_cat(t, dj > 0.0f ? "HP " : "LP ");
    hz_text(dj > 0.0f ? 20.0f * brk_exp2f(dj * 8.2f) : 18000.0f * brk_exp2f(dj * 7.8f), t + 3);   /* (common.c's) */
}

/* LOAF BREAKER: HOME's two filters, K2 the break's and K3 the drums' (SEQ's K2 and the roll pads' the same), work
 * alike: from OFF at the centre, left a low-pass, right a high-pass; away from OFF 2 % a detent, the first already heard; back towards OFF 4 % a detent (out faster
 * than in), OFF once past the first. Back at OFF it holds: turning on the same way stays OFF until the knob rests half a
 * second (no sliding into the other kind); the other way, at once. u: + right of OFF, - left; s_right, s_left: the
 * first detent's place; lk: that knob's hold */
typedef struct { uint32_t t; int8_t dir; } flt_lock_t;
static flt_lock_t lock_brk, lock_drum;
static float flt_turn(float u, int32_t d, float s_right, float s_left, flt_lock_t *lk)
{
    uint32_t now = plat_ms();
    if (u > -0.001f && u < 0.001f) {
        if (lk->dir && (d > 0) == (lk->dir > 0) && now - lk->t < 500u) {   /* still turning on past OFF: held */
            lk->t = now;
            return 0.0f;
        }
        lk->dir = 0;
        u = d > 0 ? s_right + 0.02f * (float)(d - 1) : -(s_left + 0.02f * (float)(-d - 1));
    } else if (u > 0.0f) {
        u += (d > 0 ? 0.02f : 0.04f) * (float)d;
        if (u < s_right - 0.001f) {
            u = 0.0f;
            lk->dir = -1;
            lk->t = now;
        }
    } else {
        u += (d < 0 ? 0.02f : 0.04f) * (float)d;
        if (u > -(s_left - 0.001f)) {
            u = 0.0f;
            lk->dir = 1;
            lk->t = now;
        }
    }
    return brk_clampf(u, -1.0f, 1.0f);
}

/* the drums' (set.kick_dj, -100 low-pass .. 100 high-pass): right the high-pass from 55 Hz, left the low-pass from
 * 7.6 kHz */
static void kick_flt_text(char *t)
{
    t[0] = 0;
    text_cat(t, set.kick_dj > 0 ? "HP " : set.kick_dj < 0 ? "LP " : "OFF");
    if (set.kick_dj)
        hz_text(set.kick_dj > 0 ? BRK_KICK_HP_HZ((float)set.kick_dj * 0.01f)
                                : BRK_DRUM_LP_HZ((float)set.kick_dj * 0.01f), t + 3);
}

static void kick_flt_turn(int32_t d)
{
    char t[16];
    float v = flt_turn((float)set.kick_dj * 0.01f, d, 0.02f, 0.16f, &lock_drum);
    set.kick_dj = (int8_t)(int32_t)(v * 100.0f + (v < 0.0f ? -0.5f : 0.5f));
    changed();
    kick_flt_text(t);
    app_say("DRUM FILTER", t);
}

/* the break's (set.fx.dj, -1 low-pass .. 1 high-pass): left the low-pass from 7.6 kHz, right the high-pass from
 * 110 Hz (as the drums': low-pass left, high-pass right) */
static void brk_flt_text(char *t)
{
    if (set.fx.dj < -0.01f || set.fx.dj > 0.01f)
        dj_text(set.fx.dj, t);
    else {
        t[0] = 0;
        text_cat(t, "OFF");
    }
}

static void brk_flt_turn(int32_t d)
{
    char t[16];
    set.fx.dj = flt_turn(set.fx.dj, d, 0.3f, 0.16f, &lock_brk);   /* (left the low-pass, right the high-pass) */
    changed();
    brk_flt_text(t);
    app_say("DJ FILTER", t);
}

/* the roll's decay: "DOWN 20%" each repeat quieter, "UP 20%" a swell, "OFF" */
static void roll_decay_text(float dc, char *t)
{
    char v[8];
    t[0] = 0;
    if (dc > -0.01f && dc < 0.01f) {
        text_cat(t, "OFF");
        return;
    }
    text_cat(t, dc > 0.0f ? "DOWN " : "UP ");
    pct_text(dc > 0.0f ? dc : -dc, v);
    text_cat(t, v);
}

static void pad_knob_value(int pad, int k, char *t)
{
    const brk_padset_t *p = &set.pads;
    const brk_fx_t *f = &set.fx;
    t[0] = 0;
    switch (pad <= PAD_ROLL32 ? 0 : pad) {
    case 0:
        if (k == 0) text_cat(t, BRK_ROLL_DIV_NAME[p->roll_div[pad] % BRK_ROLL_DIVS]);
        else if (k == 1) brk_flt_text(t);    /* LOAF BREAKER: HOME's two filters on K2 and K3, as there (the break's */
        else if (k == 2) kick_flt_text(t);   /* low-pass, the kick's), DECAY on K4 (the pitch and LEVEL gone) */
        else roll_decay_text(p->roll_decay, t);
        break;
    case PAD_REVERSE: times_text(p->rev_speed, t); break;
    case PAD_HALF:                       /* (GLOUPI BREAKS: K2 its master tempo) */
        if (k == 0) times_text(p->half_speed, t);
        else if (k == 1) text_cat(t, set.half_mt_off & 1u ? "OFF" : "ON");
        else text_cat(t, set.half_mt_off & 2u ? "ON" : "OFF");   /* (LOAF BREAKER: K3 LATCH) */
        break;
    case PAD_TAPESTOP:                   /* (LOAF BREAKER: the VINYL BREAK; K3 its THRU, as the ARP key's) */
        if (k == 2) { text_cat(t, THRU_NAME[set.vinyl & 3u]); break; }
        signed_tenths(k ? p->spin_steps : p->stop_beats, k ? " ST" : " BT", t);
        if (t[0] == '+') t[0] = ' ';
        break;
    case PAD_GATE:
        if (k == 0) text_cat(t, BRK_GATE_DIV_NAME[p->gate_div % BRK_GATE_DIVS]);
        else pct_text(p->gate_len, t);
        break;
    case PAD_RANDOM:
        if (k == 0) pct_text(p->rnd_amount, t);
        else { int_text((int32_t)p->rnd_pitch, t); text_cat(t, "ST"); }
        break;
    case PAD_FILTER:
        if (k == 0) dj_text(f->flt_target, t);
        else if (k == 1) pct_text(f->flt_res, t);
        else pct_text(f->flt_speed, t);
        break;
    case PAD_DUB:                        /* GLOUPI BREAKS: the ARP key: its OVERLAY effect, its LATCH */
        if (k == 0) text_cat(t, OVL_NAME[set.ovl_pad & 15u]);
        else if (k == 1) text_cat(t, THRU_NAME[(set.ovl_pad >> 6) & 3u]);
        else if (k == 2) text_cat(t, set.ovl_pad & 32u ? "OFF" : "ON");
        else text_cat(t, set.ovl_pad & 16u ? "ON" : "OFF");   /* (LOAF BREAKER: K4 LATCH) */
        break;
    default: break;
    }
}

/* THRU's knob (the ARP key's, the VINYL BREAK's): left to right KICK, BREAK, BOTH; code 0 BOTH, 1 BREAK, 2 KICK */
static uint32_t thru_turn(uint32_t code, int32_t d)
{
    static const uint8_t AT[3] = {2u, 1u, 0u}, POS[4] = {2u, 1u, 0u, 0u};   /* place <-> code */
    return AT[clamp((int32_t)POS[code & 3u] + (d > 0 ? 1 : -1), 0, 2)];
}

static void pad_knob_turn(int pad, int k, int32_t d)
{
    brk_padset_t *p = &set.pads;
    brk_fx_t *f = &set.fx;
    char t[16];
    if (pad < 0 || pad >= BRK_NPADS || k < 0 || k > 3 || !PAD_KNOB[pad][k])
        return;
    switch (pad <= PAD_ROLL32 ? 0 : pad) {
    case 0:
        if (k == 0) p->roll_div[pad] = (uint8_t)clamp((int32_t)p->roll_div[pad] + d, 0, BRK_ROLL_DIVS - 1);
        else if (k == 1 || k == 2) {     /* LOAF BREAKER: K2 the break's filter, K3 the kick's: HOME's (and SEQ's) */
            if (k == 1)
                brk_flt_turn(d);
            else
                kick_flt_turn(d);
            return;
        }
        else p->roll_decay = knob_f(p->roll_decay, d, 0.02f, -0.9f, 0.9f);
        break;
    case PAD_REVERSE: p->rev_speed = knob_f(p->rev_speed, d, 0.05f, 0.5f, 2.0f); break;
    case PAD_HALF:                       /* GLOUPI BREAKS: K2 MASTER TEMPO, right on (the pitch kept), left off */
        if (k == 0) p->half_speed = knob_f(p->half_speed, d, 0.05f, 0.25f, 0.75f);
        else if (k == 1) set.half_mt_off = (uint8_t)((set.half_mt_off & ~1u) | (d < 0 ? 1u : 0u));
        else {                           /* LOAF BREAKER: K3 LATCH, right on (held now: it stays on), left off */
            set.half_mt_off = (uint8_t)(d > 0 ? set.half_mt_off | 2u : set.half_mt_off & ~2u);
            half_lat = (uint8_t)(d > 0 && ui.pad_focus == PAD_HALF);
        }
        break;
    case PAD_TAPESTOP:
        if (k == 0)                      /* (LOAF BREAKER: to 16 beats, a beat a detent past 4) */
            p->stop_beats = knob_f(p->stop_beats, d, p->stop_beats > 4.0f || (p->stop_beats > 3.9f && d > 0) ? 1.0f
                                                                                                             : 0.25f,
                                   0.25f, 16.0f);
        else if (k == 1) p->spin_steps = knob_f(p->spin_steps, d, 0.25f, 0.25f, 8.0f);
        else set.vinyl = (uint8_t)thru_turn(set.vinyl, d);
        break;
    case PAD_GATE:
        if (k == 0) p->gate_div = (uint8_t)clamp((int32_t)p->gate_div + d, 0, BRK_GATE_DIVS - 1);
        else p->gate_len = knob_f(p->gate_len, d, 0.05f, 0.1f, 0.9f);
        break;
    case PAD_RANDOM:
        if (k == 0) p->rnd_amount = knob_f(p->rnd_amount, d, 0.05f, 0.0f, 1.0f);
        else p->rnd_pitch = knob_f(p->rnd_pitch, d, 1.0f, 0.0f, 12.0f);
        break;
    case PAD_FILTER:
        if (k == 0) f->flt_target = knob_f(f->flt_target, d, 0.04f, -1.0f, 1.0f);
        else if (k == 1) f->flt_res = knob_f(f->flt_res, d, 0.02f, 0.0f, 0.95f);
        else f->flt_speed = knob_f(f->flt_speed, d, 0.02f, 0.0f, 1.0f);
        break;
    case PAD_DUB:                        /* GLOUPI BREAKS: the ARP key. K1 which OVERLAY effect (the ARP page's
                                          * settings: amount, rate, sweep), K2 THRU, K3 ON / OFF, K4 LATCH */
        if (k == 0)
            set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | ((set.ovl_pad + (d > 0 ? 1u : 15u)) & 15u));
        else if (k == 1)                 /* K2 THRU (app.c's render) */
            set.ovl_pad = (uint8_t)((set.ovl_pad & 0x3Fu) | thru_turn(set.ovl_pad >> 6, d) << 6);
        else if (k == 2)               /* K3: ON, the key sounds its effect; OFF, K1 browses them silent */
            set.ovl_pad = (uint8_t)(d > 0 ? set.ovl_pad & ~32u : set.ovl_pad | 32u);
        else {                           /* LOAF BREAKER: K4 LATCH, right on (each press on or off), left off */
            set.ovl_pad = (uint8_t)(d > 0 ? set.ovl_pad | 16u : set.ovl_pad & ~16u);
            if (!(set.ovl_pad & 16u))
                ovlpad_lat = 0u;         /* (latched on: off with it) */
        }
        ovl_push();
        ui.redraw |= RG_BLOCKS;
        break;
    default: break;
    }
    pad_knob_value(pad, k, t);
    app_say(PAD_KNOB[pad][k], t);
    changed();
}

/* ---------------------------------------------------------- pad animations --- */
#define C_ICE RGB(150, 220, 255)
#define C_DUB RGB(120, 230, 160)
#define FX_Y 90                          /* the animation's strip: 240 x 96 where the slice blocks are */
#define FX_MID 58                        /* its waveform's centre line, in the strip */
#define FX_BAND 48                       /* drawn in two bands of 48 rows (the canvas holds 60) */

static uint32_t fx_hash(uint32_t x)      /* a stable pseudo-random number from x (the screen's dice) */
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    return x ^ (x >> 16);
}

static void fx_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* a circle of dots */
{
    uint32_t a;
    for (a = 0; a < 64u; a++) {
        float t = (float)a / 64.0f;
        cv_rect(cx + (int32_t)((float)r * brk_sin_turns(t + 0.25f)), cy + (int32_t)((float)r * brk_sin_turns(t)),
                2, 2, c);
    }
}

static void fx_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* LOAF BREAKER: a filled disc */
{
    int32_t dy, w = r;
    for (dy = 0; dy <= r; dy++) {
        while (w * w + dy * dy > r * r)
            w--;
        cv_rect(cx - w, cy - dy, 2 * w + 1, 1, c);
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}

static void fx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t n = (x1 > x0 ? x1 - x0 : x0 - x1) > (y1 > y0 ? y1 - y0 : y0 - y1) ? (x1 > x0 ? x1 - x0 : x0 - x1)
                                                                            : (y1 > y0 ? y1 - y0 : y0 - y1), i;
    for (i = 0; i <= n; i++)
        cv_rect(x0 + (x1 - x0) * i / (n ? n : 1), y0 + (y1 - y0) * i / (n ? n : 1), 2, 2, c);
}

/* a slice's shape (blk_px) stretched over w columns from x, h times as tall, in colour c */
static void fx_slice(int32_t s, int32_t x, int32_t w, float h, uint16_t c)
{
    int32_t i;
    if (s < 0 || s >= (int32_t)blk_n)
        return;
    for (i = 0; i < w; i++) {
        int32_t v = (int32_t)((float)blk_px[s][(uint32_t)i * BLK_W / (uint32_t)w] * 1.7f * h);
        cv_rect(x + i, FX_MID - v, 1, 2 * v + 1, c);
    }
}

/* the held pad's picture: what it does to the break, moving with it, and its settings */
static void draw_fx_band(int pad, const brk_status_t *st, uint32_t now, int32_t top)
{
    const brk_padset_t *p = &set.pads;
    const brk_fx_t *f = &set.fx;
    char t[24];
    int32_t i, s = st->slice >= 0 ? st->slice : 0;
    uint32_t held = now - ui.pad_since;
    cv_begin(240, FX_BAND, C_BLACK);
    cv_oy = -top;                        /* (this band's rows of the picture) */
    t[0] = 0;
    text_cat(t, PAD_TITLE[pad]);
    if (pad <= PAD_ROLL32) {
        text_cat(t, " ");
        text_cat(t, BRK_ROLL_DIV_NAME[p->roll_div[pad] % BRK_ROLL_DIVS]);
    }
    cv_text(6, 0, &FONT_B, t, C_HI);
    switch (pad <= PAD_ROLL32 ? 0 : pad) {
    case 0: {                            /* ROLL: the caught slice again and again at its level, fading or
                                          * swelling, the hit lit */
        int32_t rs = st->roll_slice >= 0 ? st->roll_slice : s, cur = (int32_t)(st->roll_n % 8u);
        float g = (float)(p->roll_level ? p->roll_level : 100u) * 0.01f;
        for (i = 0; i < 8; i++) {
            fx_slice(rs, 4 + i * 29, 27, g, i == cur ? C_HI : C_DIM);
            g = p->roll_decay >= 0.0f ? g * (1.0f - p->roll_decay) : g + (1.0f - g) * -p->roll_decay;
            if (g < 0.02f)
                g = 0.02f;
        }
        int_text((int32_t)st->roll_n, t);
        text_cat(t, " HITS");
        cv_text(234 - text_w(&FONT_XS, t), 2, &FONT_XS, t, C_GRAY);
        break;
    }
    case PAD_REVERSE: {                  /* REVERSE: the break running backwards past a fixed playhead, chevrons */
        int32_t off = (int32_t)((float)held * 0.12f * p->rev_speed) % 240;
        for (i = 0; i < 240; i++) {
            int32_t h = wave_px[(uint32_t)(240 + off - i) % 240u] * 3 / 2;
            cv_rect(i, FX_MID - h, 1, 2 * h + 1, i < 120 ? C_DIM : C_HI);
        }
        cv_rect(120, 20, 2, 76, C_WHITE);
        for (i = 0; i < 2; i++) {        /* two chevrons pointing left, sliding */
            int32_t cx = 40 + i * 22 - (int32_t)(held / 40u % 22u), r;
            for (r = 0; r < 9; r++)
                cv_rect(cx + r, FX_MID - r - 1, 3, 2, C_WHITE), cv_rect(cx + r, FX_MID + r, 3, 2, C_WHITE);
        }
        break;
    }
    case PAD_HALF: {                     /* HALF SPEED: the break stretched, crawling */
        int32_t off = (int32_t)((float)held * 0.05f * p->half_speed);
        for (i = 0; i < 240; i++) {
            int32_t h = wave_px[(uint32_t)((i + off) / 2) % 240u] * 3 / 2;
            cv_rect(i, FX_MID - h, 1, 2 * h + 1, (i / 2) % 2 ? C_HI : C_AMB);
        }
        times_text(p->half_speed, t);
        cv_text(234 - text_w(&FONT_B, t), 0, &FONT_B, t, C_WHITE);
        break;
    }
    case PAD_TAPESTOP: {                 /* LOAF BREAKER: the VINYL BREAK, a record slowing to a stop under the
                                          * tonearm (ui.reel: its turns) */
        int32_t j, cx = 92, cy = FX_MID + 2;
        fx_disc(cx, cy, 37, C_BLOCK);    /* the record, its grooves, its label */
        for (j = 33; j > 13; j -= 5)
            fx_ring(cx, cy, j, C_DIM);
        fx_disc(cx, cy, 12, C_HI);
        fx_disc(cx, cy, 2, C_BLACK);
        for (j = 0; j < 2; j++) {        /* the turning: a mark across the label, a glint in the grooves */
            float a = ui.reel + (float)j * 0.5f, sa = brk_sin_turns(a), ca = brk_sin_turns(a + 0.25f);
            fx_line(cx + (int32_t)(4.0f * ca), cy + (int32_t)(4.0f * sa), cx + (int32_t)(11.0f * ca),
                    cy + (int32_t)(11.0f * sa), C_BLACK);
            cv_rect(cx + (int32_t)(29.0f * ca) - 1, cy + (int32_t)(29.0f * sa) - 1, 3, 3, C_WHITE);
        }
        fx_disc(162, 30, 7, C_GRAY);     /* the tonearm: its pivot and weight, the arm, the head on the groove */
        cv_rect(160, 14, 5, 9, C_GRAY);
        for (j = 0; j < 3; j++)
            fx_line(161 + j, 30, 146 + j, 78, C_WHITE);
        fx_line(146, 78, 118, 84, C_WHITE);
        cv_rect(112, 80, 9, 7, C_WHITE);
        pct_text(st->tape, t);
        text_cat(t, " ");
        text_cat(t, THRU_NAME[set.vinyl & 3u]);
        cv_text(234 - text_w(&FONT_XS, t), 2, &FONT_XS, t, C_GRAY);
        break;
    }
    case PAD_GATE: {                     /* GATE: the bar's gates, open part lit, the one playing bright */
        int32_t cells = 8 << (p->gate_div % BRK_GATE_DIVS), w = 236 / cells, curc;
        float pos = (float)(st->step < 0 ? 0 : st->step % 16) + st->phase;
        curc = (int32_t)(pos * (float)cells / 16.0f) % cells;
        for (i = 0; i < cells; i++) {
            int32_t open = (int32_t)((float)(w - 1) * p->gate_len);
            cv_rect(2 + i * w, 34, w - 1, 48, C_BLOCK);
            cv_rect(2 + i * w, 34, open < 1 ? 1 : open, 48, i == curc ? (st->gate > 0.5f ? C_WHITE : C_HI) : C_AMB);
        }
        break;
    }
    case PAD_RANDOM: {                   /* RANDOM: dice over the slices, the one sounding big */
        uint32_t seed = (uint32_t)(st->step + 1) * 2654435761u;
        for (i = 0; i < 16; i++) {
            int32_t x = 120 + (i % 8) * 15, y = 30 + (i / 8) * 32;
            uint32_t r = fx_hash(seed + (uint32_t)i);
            cv_rect(x, y, 13, 28, i == s ? C_HI : (r & 3u) == 0u ? C_GRAY : C_BLOCK);
        }
        int_text(s + 1, t);
        cv_text2(20, 30, &FONT_B, t, C_WHITE);
        if (p->rnd_pitch > 0.0f)
            cv_text(20, 72, &FONT_T, "+ PITCH", C_AMB);
        break;
    }
    case PAD_FILTER: {                   /* FILTER: the response curve, its cutoff sliding to the target */
        float dj = brk_fx_dj_now(), xc = dj > 0.0f ? dj * 220.0f + 10.0f : (1.0f + dj) * 220.0f + 10.0f;
        float res = f->flt_res;
        for (i = 0; i < 240; i++) {
            float x = (float)i, d = dj > 0.0f ? xc - x : x - xc, y;
            y = d < 0.0f ? 1.0f : 1.0f / (1.0f + d * d * 0.004f);   /* past the cutoff: falls away */
            if (d > -12.0f && d < 12.0f)
                y += res * 0.8f * (1.0f - brk_absf(d) / 12.0f);     /* the resonance's bump */
            {
                int32_t h = (int32_t)(y * 34.0f);
                cv_rect(i, 92 - h, 1, h, i % 2 ? C_AMB : C_HI);
            }
        }
        cv_rect((int32_t)xc, 22, 2, 72, C_WHITE);
        dj_text(dj, t);
        cv_text(234 - text_w(&FONT_XS, t), 2, &FONT_XS, t, C_WHITE);
        break;
    }
    case PAD_DUB: {                      /* GLOUPI BREAKS: the ARP key: its effect, big, lit while on */
        int on = ovl_pad_on();
        const char *n = OVL_NAME[set.ovl_pad & 15u];
        cv_rect(6, 30, 228, 40, on ? C_HI : C_BLOCK);
        cv_text((240 - text_w(&FONT_B, n)) / 2, 38, &FONT_B, n, on ? C_BLACK : C_GRAY);
        cv_text(6, 78, &FONT_XS, set.ovl_pad & 32u ? "OFF (K3): K1 BROWSES, SILENT"
                                 : set.ovl_pad & 16u ? "LATCH: EACH PRESS ON OR OFF" : "ON WHILE HELD", C_DIM);
        (void)held;
        break;
    }
    default: break;
    }
    cv_oy = 0;
    cv_blit(0, FX_Y + (uint32_t)top);
}

/* (drawn in two bands of FX_BAND rows: the canvas is 60 rows) */
static void draw_fx(int pad, const brk_status_t *st, uint32_t now)
{
    draw_fx_band(pad, st, now, 0);
    draw_fx_band(pad, st, now, FX_BAND);
}

/* GLOUPI BREAKS: the ARP black key (PAD_DUB's, DUB itself gone): one OVERLAY effect (set.ovl_pad: bits 0-3 which,
 * bit 4 LATCH), held or switched on, over what the ARP page has on, with the ARP page's settings (ovl.c) */
static uint8_t ovlpad_held, ovlpad_lat;
static int ovl_pad_on(void) { return (ovlpad_held || ovlpad_lat) && !(set.ovl_pad & 32u); }   /* (bit 5: K3 OFF) */

/* GLOUPI BREAKS: the black key that was FREEZE (PAD_FREEZE, FREEZE itself gone): the kick's SUB through its DRIVE, on
 * and off at each press (the SUB layer's effect DRIVE or OFF; its settings, EDIT's, kept; saved with SAVE) */
static int sub_drive_on(void) { return set.kick.l[KL_SUB].fx == KFX_DRIVE; }
static void sub_drive_toggle(void)
{
    kick_layer_t *l = &set.kick.l[KL_SUB];
    if (l->fx == KFX_DRIVE)
        l->fx = KFX_OFF;
    else {
        l->fx = KFX_DRIVE;
        if (l->wet < 0.05f)              /* (a mix at nothing would make it a key that does nothing) */
            l->wet = 1.0f;
    }
    changed();
    app_say("SUB DRIVE", sub_drive_on() ? "ON" : "OFF");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
