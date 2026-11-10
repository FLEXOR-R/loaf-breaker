/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: the TUTORIAL (FLEXOR, 10 Nov 2026): the firmware's basics learnt on the FM-1 itself, a step at a time.
 * Each step says one thing to do in the header (when no other message is up) and blinks the button or keys that do
 * it; when it is done (tut_done), "OK!" and the next. Seven lessons: the break, the effects, the kick, its DISTO, the
 * pattern, the drop, SAVE. Started from SYSTEM › SETUP K1 MODE (TUTORIAL, left of BEGINNER) and on a new FM-1; GLO
 * quits it, SEL + GLO skips a step. set.tut: 0 off, 1..TUT_N the step running (kept with SAVE), TUT_DONE finished.
 * (Part of app.c's unity: included there in order, before screen.c, which draws the header.) */

enum { TC_PLAY, TC_SLICES, TC_STOP, TC_PAD, TC_PAD2, TC_EDIT, TC_KNOBS, TC_PRESETS, TC_DISTO, TC_SEQ, TC_KICKS,
       TC_SNARE, TC_DROP, TC_SAVE, TC_END };
#define TK_WHITE 1u                      /* (the keys a step blinks: the white ones, the black ones) */
#define TK_BLACK 2u
static const char *const TUT_LESSON[8] = {"THE BREAK", "EFFECTS", "THE KICK", "DISTO", "THE PATTERN", "THE DROP", "SAVE",
                                          "DONE"};
static const struct {
    uint8_t lesson, cond, keys;
    uint16_t btn;                        /* (the buttons it blinks: bit B_*) */
    const char *say;
} TUT[] = {
    {0u, TC_PLAY, 0u, 1u << B_PLAY, "PRESS PLAY: THE BREAK PLAYS"},
    {0u, TC_SLICES, TK_WHITE, 0u, "PLAY THE WHITE KEYS: ITS SLICES"},
    {0u, TC_STOP, 0u, 1u << B_PLAY, "PRESS PLAY AGAIN: IT STOPS"},
    {1u, TC_PLAY, 0u, 1u << B_PLAY, "PRESS PLAY AGAIN"},
    {1u, TC_PAD, TK_BLACK, 0u, "HOLD A BLACK KEY: AN EFFECT"},
    {1u, TC_PAD2, TK_BLACK, 0u, "NOW ANOTHER BLACK KEY"},
    {2u, TC_EDIT, 0u, 1u << B_EDIT, "PRESS EDIT: THE KICK"},
    {2u, TC_KNOBS, 0u, 0u, "TURN K1 TO K4: SHAPE IT"},
    {2u, TC_PRESETS, 0u, 0u, "TURN PRESETS: ANOTHER KICK"},
    {3u, TC_DISTO, 0u, 1u << B_SEL, "HOLD SEL, TURN K4: ITS DISTO"},
    {4u, TC_SEQ, 0u, 1u << B_SEQ, "PRESS SEQ: THE PATTERN"},
    {4u, TC_KICKS, TK_WHITE, 0u, "WHITE KEYS: PUT 2 KICKS IN"},
    {4u, TC_SNARE, TK_WHITE, 0u, "ALGORITHM TO SNARE, ADD ONE"},
    {5u, TC_DROP, 0u, 1u << B_SEL | 1u << B_ENV, "PLAYING: HOLD SEL, PRESS ENV"},
    {6u, TC_SAVE, 0u, 1u << B_SAVE, "PRESS SAVE: ALL KEPT"},
    {7u, TC_END, 0u, 0u, "THE REST: THE MANUAL. ENJOY!"},
};
#define TUT_N (sizeof TUT / sizeof TUT[0])
#define TUT_DONE 0xFEu
static int tut_on(void) { return set.tut >= 1u && set.tut <= TUT_N; }
/* what the steps watch (common.c's counters, counted where it happens: main.c, seq.c), against the step's start */
static struct {
    uint32_t wk, padm, padn, kturn, sk4, lane, saves, t;
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
    tut_at.kslot = set.kslot;
    tut_at.drop = drop_n;
    tut_at.t = plat_ms();
}
static void tut_go(uint32_t step)        /* step 1..TUT_N (past the last: finished) */
{
    set.tut = (uint8_t)(step > TUT_N ? TUT_DONE : step);
    set_dirty = 1u;
    tut_mark();
    ui.redraw |= RG_HEAD;
}
static void tut_start(void)              /* SETUP K1 left of BEGINNER: from where it was left, or the first step */
{
    tut_go(set.tut >= 1u && set.tut <= TUT_N ? set.tut : 1u);
    app_say("TUTORIAL", "GLO: QUIT, SEL+GLO: SKIP");
}
static void tut_quit(void)
{
    set.tut = TUT_DONE;
    set_dirty = 1u;
    app_say("TUTORIAL", "QUIT (SETUP K1: AGAIN)");
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
static int tut_done(uint32_t c, const brk_status_t *st)
{
    switch (c) {
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
static void tut_frame(const brk_status_t *st)   /* each frame: the step done, the next */
{
    if (!tut_on())
        return;
    if (tut_done(TUT[set.tut - 1u].cond, st)) {
        if (TUT[set.tut - 1u].cond != TC_END)
            app_say("TUTORIAL", set.tut + 1u < TUT_N ? "OK!" : "OK! ALL DONE");
        tut_go(set.tut + 1u);
    }
}
static void tut_skip(void)               /* SEL + GLO */
{
    app_say("TUTORIAL", "STEP SKIPPED");
    tut_go(set.tut + 1u);
}
static void tut_head(char *a, char *b)   /* the header's two lines: "TUTORIAL 3/16  THE BREAK", what to do */
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
    text_cat(b, TUT[s].say);
}
static void tut_leds(uint32_t *b, uint32_t *k, int playing)   /* the step's buttons and keys blinking */
{
    uint32_t s, i;
    if (!tut_on() || !((plat_ms() / 300u) & 1u))
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
