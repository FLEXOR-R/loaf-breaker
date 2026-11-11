/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: the TUTORIAL (FLEXOR, 10 Nov 2026): the firmware's basics learnt on the FM-1 itself, a step at a time.
 * Each step says one thing to do in the header (when no other message is up) and blinks the button or keys that do
 * it; when it is done (tut_done), "OK!" and the next: at once, or for a step with a result to hear (an effect, a
 * kick: its `wait`, FLEXOR 0.213) 5 s later (TUT_WAIT). Seven lessons
 * (HOME first: the PERFORM menu): the break, the effects, the kick (0.235: its BEGINNER pages, KICK and SHAPE), the
 * drums (SELECT, PRESET, FX), the pattern, the FX page, the drop, SAVE. Started from SYSTEM › SETUP K3 MODE (TUTORIAL, left of BEGINNER) and on a new FM-1; GLO
 * quits it, SEL + GLO skips a step. set.tut: 0 off, 1..TUT_N the step running (kept with SAVE), TUT_DONE finished.
 * (Part of app.c's unity: included there in order, before screen.c, which draws the header.) */

enum { TC_HOME, TC_PLAY, TC_SLICES, TC_BRK, TC_PAD, TC_ARP, TC_EDIT, TC_KPRE, TC_DISTO, TC_SHAPE, TC_TAIL, TC_PRESETS,
       TC_SNPG, TC_PPRE, TC_PFX, TC_SEQ, TC_KICKS, TC_SNARE, TC_FXPG, TC_FXT, TC_DROP, TC_SAVE, TC_END };
#define TK_WHITE 1u                      /* (the keys a step blinks: the white ones, the black ones, the ARP key) */
#define TK_BLACK 2u
#define TK_ARP 4u
#define TUT_LESSONS 9u
static const char *const TUT_LESSON[TUT_LESSONS] = {"THE BREAK", "EFFECTS", "THE KICK", "THE DRUMS", "THE PATTERN", "THE FX",
                                                    "THE DROP", "SAVE", "DONE"};
static const struct {
    uint8_t lesson, cond, keys, wait;    /* (wait: 1, its result to hear: 5 s before the next; 0: at once) */
    uint16_t btn;                        /* (the buttons it blinks: bit B_*) */
    const char *say;
} TUT[] = {                              /* (LOAF BREAKER, 0.235, FLEXOR: 23 steps, BEGINNER as he chose it: its KICK's
                                          * pages, the drums' PRESET and FX by SELECT, the FX page) */
    {0u, TC_HOME, 0u, 0u, 1u << B_HOME, "PRESS HOME: THE PERFORM MENU"},
    {0u, TC_PLAY, 0u, 1u, 1u << B_PLAY, "PRESS PLAY: THE BREAK PLAYS"},
    {0u, TC_SLICES, TK_WHITE, 1u, 0u, "PLAY THE WHITE KEYS: ITS SLICES"},
    {0u, TC_BRK, 0u, 1u, 0u, "TURN ALGORITHM: ANOTHER BREAK"},
    {1u, TC_PAD, TK_BLACK, 1u, 0u, "HOLD A BLACK KEY: AN EFFECT"},
    {1u, TC_ARP, TK_ARP, 1u, 0u, "HOLD ARP, TURN K1: ITS EFFECTS"},
    {2u, TC_EDIT, 0u, 0u, 1u << B_EDIT, "PRESS EDIT: THE KICK"},
    {2u, TC_KPRE, 0u, 1u, 0u, "TURN K1: THE BUILT-IN KICKS"},
    {2u, TC_DISTO, 0u, 1u, 0u, "TURN K2: ITS DISTO"},
    {2u, TC_SHAPE, 0u, 0u, 1u << B_EDIT, "PRESS EDIT: SHAPE, ITS TAIL"},
    {2u, TC_TAIL, 0u, 1u, 0u, "TURN K3: A LONGER TAIL"},
    {2u, TC_PRESETS, 0u, 1u, 0u, "TURN PRESETS: KICK 2 OF 10"},
    {3u, TC_SNPG, 0u, 0u, 0u, "TURN SELECT: THE SNARE"},
    {3u, TC_PPRE, 0u, 1u, 0u, "TURN K1: ITS PRESETS"},
    {3u, TC_PFX, 0u, 1u, 0u, "TURN K2: ITS FX"},
    {4u, TC_SEQ, 0u, 0u, 1u << B_SEQ, "PRESS SEQ: THE PATTERN"},
    {4u, TC_KICKS, TK_WHITE, 1u, 0u, "WHITE KEYS: PUT 2 KICKS IN"},
    {4u, TC_SNARE, TK_WHITE, 1u, 0u, "ALGORITHM TO SNARE, ADD ONE"},
    {5u, TC_FXPG, 0u, 0u, 1u << B_FX, "PRESS FX: THE EFFECTS"},
    {5u, TC_FXT, 0u, 1u, 0u, "TURN PRESETS: AN EFFECT"},
    {6u, TC_DROP, 0u, 1u, 1u << B_SEL | 1u << B_ENV, "PLAYING: HOLD SEL, PRESS ENV"},
    {7u, TC_SAVE, 0u, 0u, 1u << B_SAVE, "PRESS SAVE: ALL KEPT"},
    {8u, TC_END, 0u, 0u, 0u, "THE REST: THE MANUAL. ENJOY!"},
};
#define TUT_N (sizeof TUT / sizeof TUT[0])
#define TUT_DONE 0xFEu
#define TUT_WAIT 5000u                   /* (FLEXOR: a step done with a result to hear, 5 s before the next) */
static uint32_t tut_ok;                  /* (a step done: when, |1; 0: none) */
static int tut_on(void) { return set.tut >= 1u && set.tut <= TUT_N; }
/* what the steps watch (common.c's counters, counted where it happens: main.c, seq.c), against the step's start */
static struct {
    uint32_t wk, padm, padn, kturn, sk4, lane, saves, home, kpre, t, fx;
    uint8_t kslot, drop, brk, ovl, disto, ppre, pfx;
    float tail;
} tut_at;
static uint32_t tut_fx_sig(void)        /* (the FX chain's first two slots: their effects, presets included) */
{
    uint32_t k, s = 0u;
    for (k = 0; k < 2u; k++)
        s = s << 8 | fx_sl(k)->type | mb_preset(k) << 4 | (uint32_t)eq_magic_is(k) << 6;
    return s;
}                                /* (at the step's start) */
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
    tut_at.brk = set.brk;
    tut_at.ovl = (uint8_t)(set.ovl_pad & 15u);
    tut_at.disto = KICK_TERROR(&set.kick);
    tut_at.tail = set.kick.l[KL_SUB].decay;
    tut_at.ppre = perc_x(PV_SNARE)[PX_PRE];
    tut_at.pfx = perc_x(PV_SNARE)[PX_FX];
    tut_at.fx = tut_fx_sig();
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
/* LOAF BREAKER (FLEXOR, 11 Nov 2026, 0.229): a new FM-1 (or RESET ALL) asks first: TUTORIAL, BEGINNER or EXPERT. set.tut
 * TUT_ASK while it asks (after the start screen); a knob or ALGORITHM moves the choice, PLAY (or white key 1-3) takes
 * it. The FM-1 and the web emulator only (the simulator's scenarios start EXPERT, LOAF_ASK set: it asks) */
#define TUT_ASK 0xFDu
static uint8_t ask_sel = 1u, ask_drawn = 0xFFu;
static const char *const ASK_NAME[3] = {"TUTORIAL", "BEGINNER", "EXPERT"};
static const char *const ASK_TEXT[3][2] = {{"23 GUIDED STEPS", "THE BUTTONS TO PRESS BLINK"},
                                           {"THE ESSENTIALS", "PLAY RIGHT AWAY"},
                                           {"EVERYTHING", "EVERY SETTING, EVERY PAGE"}};
static void tut_quit(void);
static void ask_pick(uint32_t c)
{
    set.novice = c < 2u ? 1u : 0u;
    set.tut = TUT_DONE;
    if (set.novice)
        novice_lengths();
    if (c == 0u) {
        set.tut = 0u;
        tut_go(1u);
        app_say("TUTORIAL", "GLO: QUIT, SEL+GLO: SKIP");
    } else
        app_say("MODE", ASK_NAME[c % 3u]);
    set_dirty = 1u;
    changed();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = RG_ALL;
}
static void ask_draw(void)
{
    uint32_t c;
    if (ask_drawn == ask_sel)
        return;
    if (ask_drawn == 0xFFu)
        lcd_fill(0, 0, 240, 240, C_BLACK);
    ask_drawn = ask_sel;
    cv_begin(240, 60, C_BLACK);          /* the question */
    cv_text2((240 - text2_w(&FONT_B, "YOUR MODE")) / 2, 1, &FONT_B, "YOUR MODE", C_WHITE);
    cv_text((240 - text_w(&FONT_XS, "TURN A KNOB, PRESS PLAY")) / 2, 33, &FONT_XS, "TURN A KNOB, PRESS PLAY", C_AMB);
    cv_text((240 - text_w(&FONT_XS, "LATER: GLO, SETUP, K3")) / 2, 44, &FONT_XS, "LATER: GLO, SETUP, K3", C_DIM);
    cv_blit(0, 0);
    for (c = 0; c < 3u; c++) {           /* the three, one a strip */
        int on = c == ask_sel;
        char n[16];
        cv_begin(240, 60, C_BLACK);
        cv_rect(12, 3, 216, 50, on ? C_HI : C_BLOCK);
        n[0] = (char)('1' + c);
        n[1] = 0;
        cv_text(22, 9, &FONT_B, n, on ? C_BLACK : C_DIM);
        cv_text(42, 9, &FONT_B, ASK_NAME[c], on ? C_BLACK : C_WHITE);
        cv_text(42, 28, &FONT_XS, ASK_TEXT[c][0], on ? C_BLACK : C_GRAY);
        cv_text(42, 39, &FONT_XS, ASK_TEXT[c][1], on ? C_BLACK : C_GRAY);
        cv_blit(0, 60u + c * 60u);
    }
}
static void ask_input(uint32_t kdn, uint32_t bdn, int32_t d)
{
    uint32_t i;
    if (d)
        ask_sel = (uint8_t)clamp((int32_t)ask_sel + (d > 0 ? 1 : -1), 0, 2);
    for (i = 0; i < 3u; i++)
        if (kdn >> WK[i] & 1u) {
            ask_pick(i);
            return;
        }
    if (bdn & 1u << B_PLAY)
        ask_pick(ask_sel);
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
    case TC_BRK:
        return set.brk != tut_at.brk;
    case TC_PAD:
        return tut_padn != tut_at.padn;
    case TC_ARP:                         /* (the ARP key's effect changed: K1 while it is held) */
        return (set.ovl_pad & 15u) != tut_at.ovl;
    case TC_EDIT:
        return ui.page == PG_KICK;
    case TC_SHAPE:                       /* (BEGINNER's KICK, its SHAPE page) */
        return ui.page == PG_KICK && keasy_pg == 1u;
    case TC_TAIL:                        /* (three detents longer) */
        return set.kick.l[KL_SUB].decay - tut_at.tail >= 0.059f;
    case TC_SNPG:
        return ui.page == PG_PERC && ui.einst == DR_SNARE;
    case TC_PPRE:
        return perc_x(PV_SNARE)[PX_PRE] != tut_at.ppre;
    case TC_PFX:
        return perc_x(PV_SNARE)[PX_FX] != tut_at.pfx;
    case TC_FXPG:
        return ui.page == PG_FX;
    case TC_FXT:
        return tut_fx_sig() != tut_at.fx;
    case TC_KPRE:                        /* (two of them heard) */
        return tut_kpre - tut_at.kpre >= 2u;
    case TC_PRESETS:
        return set.kslot != tut_at.kslot;
    case TC_DISTO:                       /* (five detents either way: 10 %) */
        return KICK_TERROR(&set.kick) >= tut_at.disto + 10u || KICK_TERROR(&set.kick) + 10u <= tut_at.disto;
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
static void tut_head(char *a, char *b)   /* the header's two lines: "TUTORIAL 3/18  THE BREAK", what to do */
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
    text_cat(a, TUT_LESSON[TUT[s].lesson % TUT_LESSONS]);
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
    if (TUT[s].keys & TK_ARP)
        *k |= PAD_BIT(PAD_DUB);
}
