/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: the TUTORIAL (FLEXOR, 10 Nov 2026): the firmware's basics learnt on the FM-1 itself, a step at a time.
 * Each step says one thing to do in the header (when no other message is up) and blinks the button or keys that do
 * it; when it is done (tut_done), "OK!" and the next: at once, or for a step with a result to hear (an effect, a
 * kick: its `wait`, FLEXOR 0.213) 5 s later (TUT_WAIT). Seven lessons
 * (HOME first: the PERFORM menu): the break, the effects, the kick (its built-in kicks too), its DISTO, the pattern,
 * the drop, SAVE. Started from SYSTEM › SETUP K3 MODE (TUTORIAL, left of BEGINNER) and on a new FM-1; GLO
 * quits it, SEL + GLO skips a step. set.tut: 0 off, 1..TUT_N the step running (kept with SAVE), TUT_DONE finished.
 * (Part of app.c's unity: included there in order, before screen.c, which draws the header.) */

enum { TC_HOME, TC_PLAY, TC_SLICES, TC_STOP, TC_PAD, TC_PAD2, TC_EDIT, TC_KNOBS, TC_OUT, TC_KPRE, TC_PRESETS, TC_DISTO, TC_SEQ, TC_KICKS,
       TC_SNARE, TC_DROP, TC_SAVE, TC_END };
#define TK_WHITE 1u                      /* (the keys a step blinks: the white ones, the black ones) */
#define TK_BLACK 2u
static const char *const TUT_LESSON[8] = {"THE BREAK", "EFFECTS", "THE KICK", "DISTO", "THE PATTERN", "THE DROP", "SAVE",
                                          "DONE"};
static const struct {
    uint8_t lesson, cond, keys, wait;    /* (wait: 1, its result to hear: 5 s before the next; 0: at once) */
    uint16_t btn;                        /* (the buttons it blinks: bit B_*) */
    const char *say;
} TUT[] = {
    {0u, TC_HOME, 0u, 0u, 1u << B_HOME, "PRESS HOME: THE PERFORM MENU"},
    {0u, TC_PLAY, 0u, 1u, 1u << B_PLAY, "PRESS PLAY: THE BREAK PLAYS"},
    {0u, TC_SLICES, TK_WHITE, 1u, 0u, "PLAY THE WHITE KEYS: ITS SLICES"},
    {0u, TC_STOP, 0u, 0u, 1u << B_PLAY, "PRESS PLAY AGAIN: IT STOPS"},
    {1u, TC_PLAY, 0u, 0u, 1u << B_PLAY, "PRESS PLAY AGAIN"},
    {1u, TC_PAD, TK_BLACK, 1u, 0u, "HOLD A BLACK KEY: AN EFFECT"},
    {1u, TC_PAD2, TK_BLACK, 1u, 0u, "NOW ANOTHER BLACK KEY"},
    {2u, TC_EDIT, 0u, 0u, 1u << B_EDIT, "PRESS EDIT: THE KICK"},
    {2u, TC_KNOBS, 0u, 1u, 0u, "TURN K1 TO K4: SHAPE IT"},
    {2u, TC_OUT, 0u, 0u, 0u, "TURN ALGORITHM LEFT: OUT"},
    {2u, TC_KPRE, 0u, 1u, 0u, "TURN K4: THE BUILT-IN KICKS"},
    {2u, TC_PRESETS, 0u, 1u, 0u, "TURN PRESETS: KICK 2 OF 10"},
    {3u, TC_DISTO, 0u, 1u, 1u << B_SEL, "HOLD SEL, TURN K4: ITS DISTO"},
    {4u, TC_SEQ, 0u, 0u, 1u << B_SEQ, "PRESS SEQ: THE PATTERN"},
    {4u, TC_KICKS, TK_WHITE, 1u, 0u, "WHITE KEYS: PUT 2 KICKS IN"},
    {4u, TC_SNARE, TK_WHITE, 1u, 0u, "ALGORITHM TO SNARE, ADD ONE"},
    {5u, TC_DROP, 0u, 1u, 1u << B_SEL | 1u << B_ENV, "PLAYING: HOLD SEL, PRESS ENV"},
    {6u, TC_SAVE, 0u, 0u, 1u << B_SAVE, "PRESS SAVE: ALL KEPT"},
    {7u, TC_END, 0u, 0u, 0u, "THE REST: THE MANUAL. ENJOY!"},
};
#define TUT_N (sizeof TUT / sizeof TUT[0])
#define TUT_DONE 0xFEu
#define TUT_WAIT 5000u                   /* (FLEXOR: a step done with a result to hear, 5 s before the next) */
static uint32_t tut_ok;                  /* (a step done: when, |1; 0: none) */
static int tut_on(void) { return set.tut >= 1u && set.tut <= TUT_N; }
/* what the steps watch (common.c's counters, counted where it happens: main.c, seq.c), against the step's start */
static struct {
    uint32_t wk, padm, padn, kturn, sk4, lane, saves, home, kpre, t;
    uint8_t kslot, drop;
} tut_at;                                /* (at the step's start) */
static void tut_mark(void)
{
    tut_at.wk = tut_wk;
    tut_at.padm = tut_pads;
    tut_at.padn = tut_padn;
    tut_at.kturn = tut_kturn;
    tut_at.sk4 = tut_sk4;
    tut_at.lane = tut_lane;
    tut_at.saves = tut_saves;
    tut_at.home = tut_home;
    tut_at.kpre = tut_kpre;
    tut_at.kslot = set.kslot;
    tut_at.drop = drop_n;
    tut_at.t = plat_ms();
}
static void tut_go(uint32_t step)        /* step 1..TUT_N (past the last: finished) */
{
    set.tut = (uint8_t)(step > TUT_N ? TUT_DONE : step);
    set_dirty = 1u;
    tut_ok = 0u;
    tut_mark();
    ui.redraw |= RG_HEAD;
}
static void tut_start(void)              /* SETUP K3 left of BEGINNER: from where it was left, or the first step */
{
    tut_go(set.tut >= 1u && set.tut <= TUT_N ? set.tut : 1u);
    app_say("TUTORIAL", "GLO: QUIT, SEL+GLO: SKIP");
}
static void tut_quit(void)
{
    set.tut = TUT_DONE;
    set_dirty = 1u;
    app_say("TUTORIAL", "QUIT (SETUP K3: AGAIN)");
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
static int tut_done(uint32_t c, const brk_status_t *st)
{
    switch (c) {
    case TC_HOME:                        /* (HOME pressed, PERFORM: on it already, it stays: tut_home_stay) */
        return tut_home != tut_at.home && ui.page == PG_HOME;
    case TC_PLAY:
        return st->playing;
    case TC_SLICES:
        return tut_wk - tut_at.wk >= 4u;
    case TC_STOP:
        return !st->playing;
    case TC_PAD:
        return tut_padn != tut_at.padn;
    case TC_PAD2:
        return (tut_pads & ~tut_at.padm) != 0u;
    case TC_EDIT:
        return ui.page == PG_KICK;
    case TC_KNOBS:
        return tut_kturn - tut_at.kturn >= 6u;
    case TC_OUT:
        return ui.page == PG_KICK && ui.kpage == KP_OUT;
    case TC_KPRE:                        /* (two of them heard) */
        return tut_kpre - tut_at.kpre >= 2u;
    case TC_PRESETS:
        return set.kslot != tut_at.kslot;
    case TC_DISTO:
        return tut_sk4 - tut_at.sk4 >= 3u;
    case TC_SEQ:
        return ui.page == PG_SEQ;
    case TC_KICKS:
        return ui.page == PG_SEQ && sl_kick(ui.seq_lane) && tut_lane - tut_at.lane >= 2u;
    case TC_SNARE:
        return ui.page == PG_SEQ && sl_drum(ui.seq_lane) == DR_SNARE && tut_lane != tut_at.lane;
    case TC_DROP:
        return drop_n != tut_at.drop;
    case TC_SAVE:
        return tut_saves != tut_at.saves;
    default:                             /* (the last: read, then gone) */
        return plat_ms() - tut_at.t > 6000u;
    }
}
static uint32_t tut_left(void)           /* a step done: the seconds before the next (1..5) */
{
    uint32_t e = plat_ms() - tut_ok;
    return e >= TUT_WAIT ? 0u : (TUT_WAIT - e + 999u) / 1000u;
}
static void tut_frame(const brk_status_t *st)   /* each frame: the step done, 5 s, the next */
{
    static uint32_t left;
    if (!tut_on())
        return;
    if (tut_ok) {
        uint32_t l = tut_left();
        if (!l)
            tut_go(set.tut + 1u);
        else if (l != left)
            ui.redraw |= RG_HEAD;        /* (the header's countdown) */
        left = l;
        return;
    }
    if (tut_done(TUT[set.tut - 1u].cond, st)) {
        if (!TUT[set.tut - 1u].wait) {   /* (nothing to hear: "OK!" and the next at once) */
            if (TUT[set.tut - 1u].cond != TC_END)
                app_say("TUTORIAL", "OK!");
            tut_go(set.tut + 1u);
            return;
        }
        tut_ok = plat_ms() | 1u;
        left = 0u;
        ui.redraw |= RG_HEAD;
    }
}
/* HOME on PERFORM already, the TUTORIAL's HOME step: it stays there (not GENERATOR) */
static int tut_home_stay(void) { return tut_on() && !tut_ok && TUT[set.tut - 1u].cond == TC_HOME; }
static void tut_skip(void)               /* SEL + GLO */
{
    app_say("TUTORIAL", "STEP SKIPPED");
    tut_go(set.tut + 1u);
}
static void tut_head(char *a, char *b)   /* the header's two lines: "TUTORIAL 3/19  THE BREAK", what to do */
{
    char n[8];
    uint32_t s = set.tut - 1u;
    a[0] = b[0] = 0;
    text_cat(a, "TUTORIAL ");
    int_text((int32_t)set.tut, n);
    text_cat(a, n);
    text_cat(a, "/");
    int_text((int32_t)TUT_N, n);
    text_cat(a, n);
    text_cat(a, "  ");
    text_cat(a, TUT_LESSON[TUT[s].lesson % 8u]);
    if (tut_ok) {                        /* (done: "OK!  NEXT IN 4 S") */
        text_cat(b, set.tut + 1u < TUT_N ? "OK!  NEXT IN " : "OK! ALL DONE  ");
        int_text((int32_t)tut_left(), n);
        text_cat(b, n);
        text_cat(b, " S");
        return;
    }
    text_cat(b, TUT[s].say);
}
static void tut_leds(uint32_t *b, uint32_t *k, int playing)   /* the step's buttons and keys blinking */
{
    uint32_t s, i;
    if (!tut_on() || tut_ok || !((plat_ms() / 300u) & 1u))
        return;
    s = set.tut - 1u;
    *b |= TUT[s].btn;
    if (TUT[s].cond == TC_DROP && !playing)   /* (the drop: PLAY first) */
        *b |= 1u << B_PLAY;
    for (i = 0; i < 16u && TUT[s].keys & TK_WHITE; i++)
        *k |= 1u << WK[i];
    for (i = 0; i < 11u && TUT[s].keys & TK_BLACK; i++)
        *k |= 1u << BK[i];
}
