/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the pattern sequencer (SEQ), the drum lanes and REC. (Part of app.c's unity: included there in order.) */

/* ---- the pattern sequencer (SEQ) and the drum lanes: the white keys on the SEQ, KICK and SNARE / HAT pages, REC */
enum { SL_BREAK1, SL_BREAK2, SL_KICK, SL_KICK2, SL_KICK3, SL_KICK4, SL_SNARE, SL_SNARE2, SL_HAT, SL_HAT2, SL_CLAP,
       SL_CLAP2, NSEQL };                /* GLOUPI BREAKS: the kick's four bars and the snare's two, a lane each;
                                          * LOAF BREAKER: the hat's two, the clap's two */
static const char *const SEQ_LANE_NAME[NSEQL] = {"STEPS 1-16", "STEPS 17-32", "KICK 1-16", "KICK 17-32", "KICK 33-48",
                                                 "KICK 49-64", "SNARE 1-16", "SNARE 17-32", "HAT 1-16", "HAT 17-32",
                                                 "CLAP 1-16", "CLAP 17-32"};
static const char *const DRUM_NAME[NDRUMS] = {"KICK", "SNARE", "HAT", "CLAP"};
static const char *const DRUM_TAG[NDRUMS] = {"KICK", "SN", "HH", "CP"};   /* (EDIT's tabs) */
static const uint8_t SL_DR[NSEQL] = {0u, 0u, DR_KICK, DR_KICK, DR_KICK, DR_KICK, DR_SNARE, DR_SNARE, DR_HAT, DR_HAT,
                                     DR_CLAP, DR_CLAP};   /* a drum lane's drum, and its bar */
static const uint8_t SL_BAR[NSEQL] = {0u, 1u, 0u, 1u, 2u, 3u, 0u, 1u, 0u, 1u, 0u, 1u};
static int sl_kick(uint32_t sl) { return sl >= SL_KICK && sl <= SL_KICK4; }
static uint32_t sl_drum(uint32_t sl) { return SL_DR[sl % NSEQL]; }
static uint32_t sl_get(const pat_t *p, uint32_t sl)   /* the 16 steps a drum lane of SEQ (SL_KICK..) edits */
{
    return drum_bar(p, sl_drum(sl), SL_BAR[sl % NSEQL]);
}
static void sl_set(pat_t *p, uint32_t sl, uint32_t m)
{
    drum_bar_set(p, sl_drum(sl), SL_BAR[sl % NSEQL], m & 0xFFFFu);
}
static uint32_t sl_bars(const pat_t *p, uint32_t sl)   /* the bars its drum plays now: the kick 4, 2 or 1, the rest 2 or 1 */
{
    return drum_bars(p, sl_drum(sl));
}
static int sl_active(const pat_t *p, uint32_t sl)   /* a lane of SEQ the pattern has (its drum's length) */
{
    return sl < SL_KICK || SL_BAR[sl % NSEQL] < sl_bars(p, sl);
}
static int sl_now(const brk_status_t *st, uint32_t sl)   /* the step playing in that lane's 16, -1 another bar or none */
{
    const pat_t *p = &pats.p[set.pattern % NPAT];
    if (!st->playing || st->bar64 < 0 || (uint32_t)(st->bar64 >> 4) % sl_bars(p, sl) != SL_BAR[sl % NSEQL])
        return -1;
    return st->bar64 & 15;
}
static void seq_lane_fix(void)           /* a lane the pattern hasn't (a length shortened, another pattern): its first */
{
    const pat_t *p = &pats.p[set.pattern % NPAT];
    if (!sl_active(p, ui.seq_lane))
        ui.seq_lane = (uint8_t)(ui.seq_lane - SL_BAR[ui.seq_lane % NSEQL]);
}
static void seq_lane_step(int32_t d)     /* ALGORITHM / SEQ again: the next lane the pattern has */
{
    uint32_t n;
    for (n = 0; n < NSEQL; n++) {
        ui.seq_lane = (uint8_t)((ui.seq_lane + (d > 0 ? 1u : NSEQL - 1u)) % NSEQL);
        if (sl_active(&pats.p[set.pattern % NPAT], ui.seq_lane))
            break;
    }
}
static void seq_value(int i, char *t);
enum { UNDO_RANDOM = 1, UNDO_CLEAR, UNDO_LANE };   /* (UNDO_LANE + the knob) */
static void undo_mark(uint32_t what);
static uint32_t n_slices(void)
{
    const brk_break_t *b = brk_get_break();
    return b && b->nslices < 16u ? b->nslices : 16u;
}
static pat_t *pat_now(void) { return &pats.p[set.pattern % NPAT]; }

/* undo: a copy of the pattern taken before a change that wipes (SEL + ARP, SEL + REC, the drum lanes' FILL, CLEAR /
 * 4X4, COPY); a burst of the same change within 1.5 s is one. SEL + SAVE swaps it back in, and again swaps it out (redo) */
static pat_t undo_pat;
static uint8_t undo_idx = 0xFFu, undo_what;
static uint32_t undo_t;
static void undo_mark(uint32_t what)     /* what: which change (UNDO_*), a burst of the same one is one */
{
    uint32_t now = plat_ms();
    if (undo_idx != set.pattern % NPAT || what != undo_what || now - undo_t > 1500u) {
        undo_what = (uint8_t)what;
        undo_pat = *pat_now();
        undo_idx = (uint8_t)(set.pattern % NPAT);
    }
    undo_t = now;
}
static void undo_swap(void)
{
    pat_t t;
    char n[6] = "P";
    if (undo_idx >= NPAT) {
        app_say("UNDO", "NOTHING YET");
        return;
    }
    t = pats.p[undo_idx];
    pats.p[undo_idx] = undo_pat;
    undo_pat = t;
    undo_t = 0u;                         /* (the next change takes a fresh copy) */
    if (undo_idx == set.pattern % NPAT)
        pat_apply();
    set_dirty = 1u;
    int_text((int32_t)undo_idx + 1, n + 1);
    app_say("UNDO", n);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static uint32_t pat_len(void)            /* the steps the engine plays of a pattern: the break's (2 bars: 32) */
{
    const brk_break_t *b = brk_get_break();
    uint32_t n = (b ? b->bars : 2u) * BRK_STEPS_PER_BAR;
    return n < PAT_STEPS ? n : PAT_STEPS;
}
static void lane_toggle(uint32_t sl, uint32_t i)   /* sl: SL_KICK.. */
{
    uint32_t m = sl_get(pat_now(), sl) ^ 1u << i;
    sl_set(pat_now(), sl, m);
    pat_lanes_out();
    say_int(m >> i & 1u ? DRUM_NAME[sl_drum(sl)] : "OFF", (int32_t)i + 1 + 16 * (int32_t)SL_BAR[sl % NSEQL], "");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    set_dirty = 1u;
}
/* REC: the step a key or a note lands on (the one playing, or the next past its middle) */
static uint32_t rec_at(uint32_t len, int bar)
{
    brk_status_t st;
    int32_t k;
    brk_status(&st);
    k = bar ? st.bar16 : st.step;
    if (k < 0)
        k = 0;
    return ((uint32_t)k + (st.phase > 0.5f ? 1u : 0u)) % len;
}
static int playing_now(void)
{
    brk_status_t st;
    brk_status(&st);
    return st.playing;
}
static uint8_t rec_step;                 /* LOAF BREAKER: REC stopped, step by step: the step the next key writes */
static void rec_slice(uint32_t s)
{
    int live = playing_now();
    uint32_t n = pat_len(), k = live ? rec_at(n, 0) : rec_step % n;   /* (stopped: the step shown, then the next) */
    pat_now()->st[k][0] = (uint8_t)(s + 2u);
    pat_step_out(k);
    set_dirty = 1u;
    if (!live) {
        char t[12], u[4];
        rec_step = (uint8_t)((k + 1u) % n);
        int_text((int32_t)rec_step + 1, t);
        int_text((int32_t)n, u);
        text_cat(t, " / ");
        text_cat(t, u);
        app_say("REC STEP", t);
        ui.redraw |= RG_BLOCKS;
    }
}
static void midi_slice(int s, int on)    /* a note 48..63: as the white key, recorded with REC */
{
    if (on && ui.rec)
        rec_slice((uint32_t)s);
    brk_key(s, on);
}
static void midi_drum(uint32_t d)        /* a drum note (DR_*): the hit now, and with REC into its lane */
{
    if (d == DR_KICK)
        kick_hit();
    else
        perc_hit(d - 1u);
    if (ui.rec && playing_now()) {
        brk_status_t st;
        uint32_t k, b;
        brk_status(&st);                 /* (into its bar playing) */
        k = ((uint32_t)(st.bar64 < 0 ? 0 : st.bar64) + (st.phase > 0.5f ? 1u : 0u)) % 64u;
        b = (k >> 4) % drum_bars(pat_now(), d);
        drum_bar_set(pat_now(), d, b, drum_bar(pat_now(), d, b) | 1u << (k & 15u));
        pat_lanes_out();
        set_dirty = 1u;
    }
}
/* K1-K4 on a drum lane (SEQ): FILL n hits spread evenly (Euclidean, from the downbeat), KICK FLT (LOAF BREAKER, in
 * place of ROTATE: the kick's DJ filter, HOME's K3's),
 * CLEAR (left) or four on the floor (right), LENGTH (GLOUPI BREAKS, in place of COPY: the kick 16, 32 or 64 steps, the
 * snare 16 or 32) */
static void lane_knob(int i, int32_t d)
{
    static const char *const NAME[4] = {"FILL", "KICK FILTER", "LANE", "LENGTH"};
    uint32_t m = sl_get(pat_now(), ui.seq_lane), n = 0u, k;
    char t[16];
    if (i == 1) {                        /* LOAF BREAKER: KICK FLT, the kick's high-pass (HOME's K3: the same) */
        kick_flt_turn(d);
        ui.redraw |= RG_FOOT;
        return;
    }
    if (i == 3) {                        /* GLOUPI BREAKS: LENGTH, the kick 16, 32 or 64 steps, the snare 16 or 32 */
        uint8_t *len = &PAT_LEN(pat_now());
        if (sl_kick(ui.seq_lane)) {      /* the kick: 16, 32 or 64 steps, right longer */
            uint32_t at = kick_bars(pat_now()) == 1u ? 0u : kick_bars(pat_now()) == 2u ? 1u : 2u;
            at = (uint32_t)clamp((int32_t)at + (d > 0 ? 1 : -1), 0, 2);
            *len = (uint8_t)((*len & ~(PAT_KICK16 | PAT_KICK32)) | (at == 0u ? PAT_KICK16 : at == 1u ? PAT_KICK32 : 0u));
        } else                           /* the snare, the hat, the clap: 16 or 32, right longer (LOAF BREAKER) */
            *len = (uint8_t)(d > 0 ? *len | LEN_BIT[sl_drum(ui.seq_lane)] : *len & ~LEN_BIT[sl_drum(ui.seq_lane)]);
        pat_lanes_out();
        seq_lane_fix();
        set_dirty = 1u;
        seq_value(i, t);
        app_say(NAME[i], t);
        ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
        return;
    }
    undo_mark(UNDO_LANE + (uint32_t)i);
    if (i == 0) {
        for (k = m; k; k >>= 1)
            n += k & 1u;
        n = (uint32_t)clamp((int32_t)n + (d > 0 ? 1 : -1), 0, 16);
        for (m = 0u, k = 0u; k < 16u; k++)
            if (k * n % 16u < n)
                m |= 1u << k;
    } else
        m = d > 0 ? 0x1111u : 0u;
    sl_set(pat_now(), ui.seq_lane, m);
    pat_lanes_out();
    set_dirty = 1u;
    seq_value(i, t);
    app_say(NAME[i], i == 2 ? (m ? "4X4" : "CLEARED") : t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void seq_key(uint32_t i)          /* a white key on SEQ: the step to edit, or a drum step on and off */
{
    char t[8];
    seq_lane_fix();
    if (ui.seq_lane >= SL_KICK) {
        lane_toggle(ui.seq_lane, i);
        return;
    }
    ui.seq_step = (uint8_t)(ui.seq_lane * 16u + i);
    seq_value(0, t);
    say_int("STEP", (int32_t)ui.seq_step + 1, "");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* SEL + REC: 0 the whole pattern (elsewhere), 1 on SEQ the lane shown; LOAF BREAKER: 2 REC held on SEQ, the break's
 * steps, both halves (1-16, 17-32), the drums kept */
static void seq_clear(int mode)
{
    pat_t *p = pat_now();
    uint32_t k;
    undo_mark(UNDO_CLEAR);
    if (mode != 1 || ui.seq_lane < SL_KICK)
        for (k = 0; k < PAT_STEPS; k++)
            if (mode != 1 || k / 16u == ui.seq_lane) {
                p->st[k][0] = 1u;
                p->st[k][1] = p->st[k][2] = 0u;
            }
    if (!mode)
        for (k = SL_KICK; k < NSEQL; k++)
            sl_set(p, k, 0u);
    else if (mode == 1 && ui.seq_lane >= SL_KICK)
        sl_set(p, ui.seq_lane, 0u);
    pat_apply();
    set_dirty = 1u;
    app_say("CLEARED", mode == 1 ? SEQ_LANE_NAME[ui.seq_lane] : mode ? "BREAK 1-32" : "PATTERN");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void seq_value(int i, char *t)    /* the step edited: SLICE, PITCH, HITS, REVERSE */
{
    const uint8_t *b = pat_now()->st[ui.seq_step % PAT_STEPS];
    t[0] = 0;
    if (ui.seq_lane >= SL_KICK) {        /* a drum lane: FILL (its hits), KICK FLT, CLEAR / 4X4, LENGTH */
        uint32_t m = sl_get(pat_now(), ui.seq_lane), n = 0u;
        while (m) {
            n += m & 1u;
            m >>= 1;
        }
        if (i == 0) {
            int_text((int32_t)n, t);
            text_cat(t, " HITS");
        } else if (i == 1)
            kick_flt_text(t);            /* (as on HOME) */
        else if (i == 2)
            text_cat(t, "L CLR R 4X4");
        else {
            int_text((int32_t)(16u * sl_bars(pat_now(), ui.seq_lane)), t);
            text_cat(t, " STEPS");
        }
        return;
    }
    if (i == 0) {
        if (b[0] < 2u)
            text_cat(t, b[0] ? "LOOP" : "OFF");
        else
            int_text(b[0] - 1, t);
    } else if (i == 1) {
        int_text((int8_t)b[1], t);
        text_cat(t, " ST");
    } else if (i == 2) {
        text_cat(t, "X");
        int_text(((b[2] >> 1) & 3u) + 1, t + 1);
    } else
        text_cat(t, b[2] & 1u ? "REVERSE" : "FORWARD");
}
static void seq_turn(int i, int32_t d)
{
    uint8_t *b = pat_now()->st[ui.seq_step % PAT_STEPS];
    char t[12];
    seq_lane_fix();
    if (ui.seq_lane >= SL_KICK) {
        lane_knob(i, d);
        return;
    }
    if (i == 0)                          /* OFF, LOOP (follow the break), slices 1.. */
        b[0] = (uint8_t)clamp((int32_t)b[0] + d, 0, (int32_t)(n_slices() + 1u));
    else if (i == 1)
        b[1] = (uint8_t)(int8_t)clamp((int8_t)b[1] + d, -12, 12);
    else if (i == 2)
        b[2] = (uint8_t)((b[2] & 1u) | (uint32_t)clamp((int32_t)((b[2] >> 1) & 3u) + (d > 0 ? 1 : -1), 0, 3) << 1);
    else
        b[2] = (uint8_t)((b[2] & ~1u) | (d > 0 ? 1u : 0u));
    pat_step_out(ui.seq_step % PAT_STEPS);
    set_dirty = 1u;
    seq_value(i, t);
    say_int("STEP", (int32_t)ui.seq_step + 1, "");
    app_say(ui.msg[0], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;    /* (the knobs' values too: they show the step) */
}
static void pattern_pick(int32_t d)      /* PRESETS: the next or previous pattern (playing: at the loop's end) */
{
    int32_t cur = pat_next >= 0 ? pat_next : set.pattern;
    int32_t want = (cur + (d > 0 ? 1 : (int32_t)NPAT - 1)) % (int32_t)NPAT;
    if (playing_now())
        pat_next = (int8_t)want;
    else {
        set.pattern = (uint8_t)want;
        pat_apply();
        set_dirty = 1u;
    }
    say_int(playing_now() ? "NEXT PATTERN" : "PATTERN", want + 1, "");
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
static uint32_t keys_lit(const brk_status_t *st)   /* the white keys' lights on a lane's page (TAP: the beat) */
{
    if (ui.page == PG_TAP)
        return st->playing && st->bar16 >= 0 && !(st->bar16 & 3) ? 0xFFFFu : 0u;
    if (ui.page == PG_OVL)               /* ARP: the overlays on */
        return ovl_held | ovl_latched;
    uint32_t m = 0u, i;
    if (ui.seq_lane >= SL_KICK) {
        int now = sl_now(st, ui.seq_lane);
        m = sl_get(pat_now(), ui.seq_lane);
        if (now >= 0)
            m ^= 1u << now;
        return m;
    }
    for (i = 0; i < 16u; i++)            /* SEQ's break steps: the ones with a slice of their own */
        if (pat_now()->st[ui.seq_lane * 16u + i][0] >= 2u)
            m |= 1u << i;
    if (st->playing && st->step >= 0 && (uint32_t)st->step / 16u == ui.seq_lane)
        m ^= 1u << (st->step % 16);
    return m;
}
/* LOAF BREAKER: the SEQ page, everything at once. Under the tabs (y 44..191, drawn in three bands), a row of 16 steps
 * each: the break's two halves (each step its slice, LOOP a dot, off a dash; the one edited outlined), then every drum
 * over the length it has (the kick's 1, 2 or 4 bars, the snare's, hat's and clap's 1 or 2), named with that length
 * ("K64", "SN32"; its other bars "2", "3", "4"). The rows share the height: fewer rows, bigger steps. The step playing
 * white; the lane the keys edit named lit, a line under it */
#define SQ_H 148                         /* the picture's rows (screen 44..191) */
#define SQ_X 30                          /* the steps begin (the names before) */
static uint32_t seq_rows(uint8_t *row)   /* the rows, top to bottom (SL_*); their count */
{
    uint32_t sl, n = 0u;
    for (sl = 0; sl < NSEQL; sl++)
        if (sl >= SL_KICK ? sl_active(pat_now(), sl) : sl * 16u < pat_len())
            row[n++] = (uint8_t)sl;
    return n;
}
static void seq_picture(const brk_status_t *st, int32_t top)
{
    static const char *const TAG[NDRUMS] = {"K", "SN", "HH", "CP"};
    const pat_t *p = pat_now();
    uint8_t row[NSEQL];
    uint32_t i, r, n = seq_rows(row);
    int32_t pitch = SQ_H / (int32_t)(n ? n : 1u), ch = pitch > 4 ? pitch - 2 : pitch;
    char t[8];
    cv_oy = -top;
    for (r = 0; r < n; r++) {
        uint32_t sl = row[r], brk = sl < SL_KICK, m = brk ? 0u : sl_get(p, sl);
        int32_t y = (int32_t)r * pitch, at = brk ? -1 : sl_now(st, sl);
        int picked = sl == ui.seq_lane;
        t[0] = 0;                        /* the row's name: the drum and its length on its first bar, else the bar */
        if (brk)
            text_cat(t, sl ? "2" : "BRK");
        else if (SL_BAR[sl]) {
            t[0] = (char)('1' + SL_BAR[sl]);
            t[1] = 0;
        } else {
            char u[4];
            int_text((int32_t)(16u * sl_bars(p, sl)), u);
            text_cat(t, TAG[sl_drum(sl)]);
            text_cat(t, u);
        }
        cv_text(SQ_X - 3 - text_w(&FONT_XS, t), y + (ch - 8) / 2, &FONT_XS, t, picked ? C_HI : brk || !SL_BAR[sl] ? C_GRAY
                : C_DIM);
        for (i = 0; i < 16u; i++) {
            int32_t x = SQ_X + (int32_t)(i * (240u - SQ_X) / 16u), w = SQ_X + (int32_t)((i + 1u) * (240u - SQ_X) / 16u) - x - 2;
            uint16_t c;
            if (brk) {                   /* a step of the break: its slice */
                uint32_t k = sl * 16u + i, v = p->st[k][0];
                int now = st->playing && st->step == (int32_t)k, sel = ui.seq_lane < SL_KICK && k == ui.seq_step;
                cv_rect(x, y, w, ch, sel ? C_WHITE : now ? C_BLOCK_HI : (i & 3u) ? C_BLOCK : C_LINE);
                if (sel && ch > 4)
                    cv_rect(x + 1, y + 1, w - 2, ch - 2, now ? C_BLOCK_HI : C_BLOCK);
                t[0] = 0;
                if (v >= 2u)
                    int_text((int32_t)v - 1, t);
                else
                    text_cat(t, v ? "." : "-");
                if (ch >= 9)
                    cv_text(x + (w - text_w(&FONT_XS, t)) / 2, y + (ch - 8) / 2, &FONT_XS, t,
                            now ? C_BLACK : v >= 2u ? C_HI : C_GRAY);
                else if (v >= 2u)        /* (too small for its number: a mark) */
                    cv_rect(x + 1, y + ch / 2, w - 2, 1, now ? C_BLACK : C_HI);
                if (p->st[k][2] & 7u && ch >= 6)   /* reversed or ratcheted: a mark */
                    cv_rect(x + 2, y + ch - 2, w - 4, 1, now ? C_BLACK : C_AMB);
                continue;
            }
            c = at == (int32_t)i ? C_WHITE : m >> i & 1u ? (picked ? C_BLOCK_HI : C_AMB) : (i & 3u) ? C_BLOCK : C_LINE;
            cv_rect(x, y, w, ch, c);
            if (at == (int32_t)i && ch > 6)
                cv_rect(x + 2, y + 2, w - 4, ch - 4, m >> i & 1u ? C_BLOCK_HI : C_BLOCK);
        }
        if (picked && pitch - ch >= 2)   /* the lane the keys edit: a line under it */
            cv_rect(SQ_X, y + ch, 240 - SQ_X - 2, 1, C_HI);
    }
    cv_oy = 0;
}
static void draw_seq(const brk_status_t *st)
{
    int32_t top;
    seq_lane_fix();
    for (top = 0; top < SQ_H; top += 60) {   /* (the canvas holds 60 rows) */
        int32_t h = SQ_H - top < 60 ? SQ_H - top : 60;
        cv_begin(240, (uint32_t)h, C_BLACK);
        seq_picture(st, top);
        cv_blit(0, (uint32_t)(44 + top));
    }
}
