/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: ENV, where TAP TEMPO was: TRACKS (the patterns in an order, their loops, DROPs between them), PLAY
 * (a track from its start, or from an item: back to HOME as it starts) and DROP (the build's bars, the pattern after
 * the drop, the drop's break and crash). ENV again: the next part. The track plays from the main loop: as a pattern
 * comes into the last bar of its last loop, the next item is asked for (a pattern: in at the loop's end, pat_next; a
 * DROP: the build from the next bar, then the drop into the pattern after it). (Part of app.c's unity: included
 * there in order.) */

static void page_say(void);
static void break_ask(uint32_t i);      /* (main.c) */
#define NTPART 6u
static const char *const TPART[NTPART] = {"TRACKS", "PLAY", "DROP", "KNOB", "BUILD", "RESET"};
#define NTORD 4u
static const uint8_t TORDER[NTORD] = {0u, 1u, 2u, 4u};   /* (ENV's parts in order: BUILD after DROP; LOAF BREAKER TEST:
                                          * KNOB out (EDIT KICK's KNOBS has it all), RESET on GLO, its sixth part) */
#define NGPART 6u                        /* (GLO's parts: MIDI, PITCH, MORE, EFFECTS, FX SET, RESET) */
#define RST_ON (ui.page == PG_GLOBAL && ui.gpage == 5u)   /* (GLO RESET: drawn and turned by ENV's code here) */
/* RESET (ENV's last part): K1 what (a list: more to come), K4 turned right twice within 3 s: back to the start */
enum { RST_DROP, RST_BUILD, RST_KNOB, RST_KICK, RST_KTONE, RST_KICKS, RST_DRUMS, RST_MIX, RST_FX, RST_STEPS, RST_PADS,
       RST_KEYS, RST_TRACK, RST_SCREEN, NRST };
static const char *const RST_NAME[NRST] = {"DROP", "BUILD", "KNOB", "KICK", "KICK TONE", "ALL KICKS", "DRUMS", "MIX",
                                           "FX CHAIN", "STEPS", "PADS", "KEYS", "TRACK", "SCREEN"};
static const char *const RST_WHAT[NRST] = {"BUILD 4 BARS, NEXT PATTERN, SAME BREAK, CRASH 100%",
                                          "SNARE ROLL, HIGH-PASS, RISER 100%, NO GAP",
                                          "THIS KICK'S: K1 DRIVE, K2 TERROR 2, K3 T+ SCREECH, K4 TERROR",
                                          "THE KICK SELECTED (ITS MEMORY): ROTTERDAM, SYNTH, BASS, C SUB, MORPH AT 0, VOLUME KEPT",
                                          "THE KICK'S EQ FLAT, ITS TWO FX SLOTS EMPTY",
                                          "KICK 1-10 AS THEY CAME (909 .. DISTORTO), SYNTH AT 0, THEIR KNOBS AS THEY START",
                                          "SNARE, HAT, CLAP, HOOVER: THE MEMORY PLAYING AS IT CAME",
                                          "THE MIXER: LEVELS, SWING, COMP AND DUCK OFF, MAX OUT -3 DB",
                                          "THE FOUR SLOTS AS THEY CAME, EACH EFFECT'S MEMORY FORGOTTEN",
                                          "THIS PATTERN'S DRUM STEPS: 1 HIT, ALWAYS, ON TIME",
                                          "THE BLACK KEYS: ROLLS, THRU, FILTER, GATE, LATCH, THE ARP KEY",
                                          "THE WHITE KEYS' LOOPS AND SLICE EFFECTS",
                                          "THIS TRACK: FROM ITEM 1, EVERY ITEM'S BREAK SAME",
                                          "SKIN RED, SCREEN EFFECTS OFF, THEIR SETTINGS 50%, WAVE ZOOM 1 BAR"};
#define RST_TURNS 5u                     /* LOAF BREAKER: the clicks that reset (K2, K3 or K4), each within 2 s of the last */
static int rst_live(void)
{
    return ui.rst_n && ui.rst_arm && plat_ms() - ui.rst_arm < 2000u;
}
static void reset_pick(int32_t d)        /* RESET: the part (K1 or ALGORITHM); the clicks counted start again */
{
    ui.rst_sel = (uint8_t)clamp((int32_t)ui.rst_sel + (d > 0 ? 1 : -1), 0, NRST - 1);
    ui.rst_arm = 0u;
    ui.rst_n = 0u;
    app_say("RESET", RST_NAME[ui.rst_sel % NRST]);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void reset_do(uint32_t w)
{
    if (w == RST_DROP) {
        set.drop_bars = 3u;
        set.drop_after = 0u;
        set.drop_brk = 0u;
        set.drop_crash = 100u;
    } else if (w == RST_BUILD) {
        set.drop_style = DS_SNARE;
        set.drop_sweep = 0u;
        set.drop_riser = 50u;
        set.drop_gap = 0u;
    } else if (w == RST_KNOB)            /* KNOB: the knobs' functions */
        set.selk_asg = set.selk_asg2 = 0u;
    else if (w == RST_KTONE) {           /* KICK TONE: its EQ flat, its FX slots empty, their memory forgotten */
        uint32_t k;
        for (k = 0; k < 8u; k++)
            set.keq[k] = 0.5f;
        for (k = 0; k < BRK_KFX_SLOTS; k++) {
            brk_fx_slot_defaults(&set.kfx[k], FXT_EMPTY);
            set.kfx[k].on = 0u;
            brk_fxx_defaults(&set.kfxx[k], FXT_EMPTY);
            brk_mbc_defaults(&set.kmbc[k]);
        }
        pats.kfm_ok = 0u;
    } else if (w == RST_KICKS) {         /* ALL KICKS: KICK 1-10 as they came, the one selected with them */
        float vol = set.kick.volume;
        uint32_t k, j;
        for (k = 0; k < KICK_MEMS; k++) {
            kick_preset(&set.kmem[k], k % KICK_PRESETS);
            set.kmem[k].volume = vol;
            for (j = 0; j < KS_N; j++)
                set.ksmem[k][j] = 0;
        }
        set.kick = set.kmem[set.kslot % KICK_MEMS];
        for (j = 0; j < KS_N; j++)
            set.ksyn[j] = 0;
        set.selk_asg = set.selk_asg2 = set.csub = set.kmorph = set.kmorph_to = 0u;   /* (their knobs as they start) */
        set.kbass[0] = set.kbass[1] = 0u;
        set.kbass[2] = set.kbass[3] = 50u;
        kx_all();
    } else if (w == RST_DRUMS) {         /* DRUMS: each drum's memory playing as it came (its preset) */
        uint32_t v;
        for (v = 0; v < PERC_VOICES; v++) {
            uint32_t m = *perc_slot(v) % PERC_MEMS;
            perc_preset(perc_mem(v, m), v, m % PERC_PRESETS);
            *perc_voice(v) = *perc_mem(v, m);
        }
    } else if (w == RST_MIX) {           /* MIX: the MIXER's four parts as they start */
        brk_fx_t f;
        perc_t d;
        brk_fx_defaults(&f);
        perc_defaults(&d);
        set.brk_level = 100u;
        set.kick.volume = 0.8f;
        set.perc.volume = d.volume;
        set.fx.wet = f.wet;
        set.swing = 50u;
        set.drums_chain = 1u;
        set.comp = set.duck = set.duck_what = set.max_out = 0u;
        set.comp_ratio = 3u;
        set.comp_att = 59u;
        set.comp_rel = 55u;
        set.duck_att = 19u;
        set.duck_rel = 36u;
    } else if (w == RST_FX) {            /* FX CHAIN: its slots as they came (the MIXER's FX level kept) */
        float wet = set.fx.wet;
        uint32_t k;
        brk_fx_defaults(&set.fx);
        set.fx.wet = wet;
        for (k = 0; k < BRK_FX_SLOTS; k++) {
            brk_mbc_defaults(&set.mbc[k]);
            brk_fxx_defaults(&set.fxx[k], set.fx.slot[k].type);
        }
        pats.mem.fx_ok = 0u;
    } else if (w == RST_STEPS) {         /* STEPS: this pattern's drum steps plain (the steps themselves kept) */
        uint32_t k;
        for (k = 0; k < STEPB_N; k++)
            steps.b[set.pattern % NPAT][k] = 0u;
        for (k = 0; k < NDRUMS; k++)
            pats.hits[set.pattern % NPAT][k] = 0u;
    } else if (w == RST_PADS) {          /* PADS: the black keys' settings as they came */
        uint32_t k;
        brk_padset_defaults(&set.pads);
        set.half_mt_off = 0u;
        set.ovl_pad = 0u;
        for (k = 0; k < 16u; k++)
            set.ovl_amt[k] = k == OV_DIVE || k == OV_RISE || k == OV_CRUSH || k == OV_DIST || k == OV_BOOM ? 70u : 100u;
        set.ovl_rate = 2u;
        set.ovl_sweep = 2u;
        set.ovl_latch = 0u;
    } else if (w == RST_KEYS) {          /* KEYS: each white key's loop and slice effect as they came */
        uint32_t k;
        for (k = 0; k < BRK_KEYS; k++) {
            brk_keyset_defaults(&set.keys[k]);
            brk_slicefx_defaults(&set.slx[k]);
        }
    } else if (w == RST_TRACK) {         /* TRACK: this track from its first item, each item's break SAME (its items
                                          * kept) */
        uint32_t k;
        pats.trk[set.trk % NTRACKS].rsv[0] = 0u;   /* (TRK_FROM, below) */
        for (k = 0; k < TRK_ITEMS; k++)
            steps.tb[set.trk % NTRACKS][k] = 0u;
    } else if (w == RST_SCREEN) {        /* SCREEN: the skin, the screen effects and their settings, WAVE ZOOM as they
                                          * start (GLO MORE, EFFECTS, FX SET) */
        set.skin = 0u;
        set.sfx_on = 0u;
        set.scrfx = set.sfx_kick = set.sfx_drop = set.sfx_len = 50u;
        set.wzoom = 3u;
        ui.redraw = RG_ALL;
    } else {                               /* LOAF BREAKER: KICK: the kick selected (KICK 1-10) as it starts: ROTTERDAM,
                                          * its TERRORs and effects with it, its SYNTH at 0, its BASS, CLEAN SUB
                                          * and MORPH off; its volume
                                          * kept; its memory with it */
        float vol = set.kick.volume;
        uint32_t j;
        kick_defaults(&set.kick);
        set.kick.volume = vol;
        for (j = 0; j < KS_N; j++)
            set.ksyn[j] = 0;
        set.kbass[0] = set.kbass[1] = 0u;
        set.kbass[2] = set.kbass[3] = 50u;
        set.csub = set.kmorph = set.kmorph_to = 0u;   /* (CLEAN SUB and MORPH off; ENV KNOB's choices: RESET KNOB) */
        set.kmem[set.kslot % KICK_MEMS] = set.kick;
        for (j = 0; j < KS_N; j++)
            set.ksmem[set.kslot % KICK_MEMS][j] = 0;
    }
    changed();
}
static const char *const DS_NAME[DS_N] = {"SNARE ROLL", "KICK ROLL", "KICK+SNARE", "STUTTER", "SWEEP ONLY", "RANDOM"};
static const char *const SWEEP_NAME[4] = {"HIGH-PASS", "LOW-PASS", "NONE", "RANDOM"};
static uint32_t drop_rng = 0x2545F491u;
static uint32_t drop_rand(uint32_t n)    /* 0..n-1, by chance (the drops unlike each other) */
{
    drop_rng = drop_rng * 1664525u + 1013904223u + plat_ms();
    return (drop_rng >> 16) % n;
}
static const char *const TRK_END_NAME[3] = {"LOOP", "STOP", "HOLD"};

static track_t *trk_now(void) { return &pats.trk[set.trk % NTRACKS]; }
#define TRK_FROM (trk_now()->rsv[0])     /* LOAF BREAKER: the track's first item (TRACKS: SEL + PLAY; PLAY's FROM), saved
                                          * with it */
static uint8_t *trk_brk(uint32_t i) { return &steps.tb[set.trk % NTRACKS][i % TRK_ITEMS]; }   /* (the item's break) */
static int trk_is_pat(const track_t *t, uint32_t i)
{
    uint32_t v = t->it[i % TRK_ITEMS];
    return v != TI_END && v != TI_DROP && v != TI_GAP;
}
static void trk_brk_go(uint32_t i)       /* a pattern item coming: its own break, if it has one (at the bar's end) */
{
    uint32_t b = *trk_brk(i);
    if (b && b <= n_breaks && b - 1u != set.brk)
        break_ask(b - 1u);
}
static uint32_t trk_len(const track_t *t)   /* its items before the END */
{
    uint32_t n = 0u;
    while (n < TRK_ITEMS && t->it[n] != TI_END)
        n++;
    return n;
}
static uint32_t trk_bars(const track_t *t)   /* how long it plays, in bars (the drops' builds counted) */
{
    uint32_t i, b = 0u, n = trk_len(t), lb = pat_len() / 16u;
    for (i = 0; i < n; i++)
        b += t->it[i] == TI_DROP ? DROP_BARS[set.drop_bars % 5u] : t->it[i] == TI_GAP ? 0u : (uint32_t)t->rep[i] * lb;
    return b;
}
/* LOAF BREAKER TEST: a track playing, where it is in bars (done of total), and the bars left before its next DROP
 * (-1: none next); 0 when no track plays */
static int trk_where(const brk_status_t *st, uint32_t *done, uint32_t *total, int32_t *drop_in)
{
    const track_t *t = trk_now();
    uint32_t i, b = 0u, lb = pat_len() / 16u, pos = ui.trk_pos % TRK_ITEMS, n = trk_len(t), in, item;
    if (!ui.trk_on || !n)
        return 0;
    if (!lb)
        lb = 1u;
    for (i = 0; i < pos && i < n; i++)
        b += t->it[i] == TI_DROP ? DROP_BARS[set.drop_bars % 5u] : t->it[i] == TI_GAP ? 0u : (uint32_t)t->rep[i] * lb;
    item = t->it[pos] == TI_DROP ? DROP_BARS[set.drop_bars % 5u] : (uint32_t)t->rep[pos] * lb;
    in = t->it[pos] == TI_DROP ? 0u : (uint32_t)ui.trk_loops * lb + (st->step >= 0 ? (uint32_t)st->step / 16u : 0u);
    if (in > item)
        in = item;
    *done = b + in;
    *total = trk_bars(t);
    *drop_in = -1;
    for (i = pos + 1u; i < n && t->it[i] == TI_GAP; i++)
        ;
    if (i < n && t->it[i] == TI_DROP && t->it[pos] != TI_DROP)
        *drop_in = (int32_t)(item - in);
    return 1;
}
static int trk_pat_from(const track_t *t, uint32_t i, int wrap)   /* the first pattern item at i or after (wrap: from
                                          * the start again past the END), -1 none */
{
    uint32_t n = trk_len(t), k;
    for (k = 0; k < (wrap ? 2u * TRK_ITEMS : TRK_ITEMS); k++, i++) {
        if (i >= n) {
            if (!wrap || !n)
                return -1;
            i = 0u;
        }
        if (t->it[i] != TI_DROP && t->it[i] != TI_GAP)
            return (int)i;
    }
    return -1;
}

/* the drop asked for: a build into pattern to (its drums ready for the render), or (building) the drop at the
 * next bar */
static void drop_start(uint32_t to)
{
    if (!playing_now()) {
        app_say("DROP", "PLAY FIRST");
        return;
    }
    if (build_p < 0.0f && !build_wait) {
        const pat_t *p;
        uint32_t d, b;
        char t[8] = "> P";
        ui.drop_pat = (uint8_t)(to % NPAT);
        drop_to = ui.drop_pat;
        p = &pats.p[ui.drop_pat];
        for (d = 0; d < NDRUMS; d++)
            for (b = 0; b < 4u; b++)
                drum_next[d][b] = (uint16_t)drum_bar(p, d, b % drum_bars(p, d));
        drum_next_ok = 1u;
        build_bars = DROP_BARS[set.drop_bars % 5u];
        drop_gain = (float)set.drop_crash * 0.01f;
        b_style = (uint8_t)(set.drop_style < DS_RANDOM ? set.drop_style : drop_rand(DS_RANDOM));   /* (BUILD) */
        b_sweep = (uint8_t)(set.drop_sweep < 3u ? set.drop_sweep : drop_rand(3u));
        b_riser = (float)(set.drop_riser > 100u ? 100u : set.drop_riser) * 0.02f;
        b_gap = DROP_GAP[set.drop_gap % 5u];
        ui.build_said = plat_ms();
        ui.drop_early = ui.drop_asked = 0u;
        int_text((int32_t)ui.drop_pat + 1, t + 3);
        app_say(build_bars ? "BUILD NEXT BAR" : "DROP AT BAR END", t);
    } else if (build_p >= 0.0f) {
        app_say("DROP", "NEXT BAR");
        ui.drop_early = 1u;
    } else
        return;                          /* (asked already: it comes) */
    build_n++;
}
/* SEL clicked three times: the drop, into DROP's AFTER (a track playing: its next pattern, the track on from it) */
static void build_click(void)
{
    if (ui.trk_on && build_p < 0.0f && !build_wait) {
        const track_t *t = trk_now();
        int i = trk_pat_from(t, ui.trk_pos + 1u, t->end == 0u);
        if (i >= 0) {
            ui.trk_drop = (uint8_t)i;
            drop_start((uint32_t)t->it[i] - 1u);
            return;
        }
    }
    drop_start(set.drop_after ? set.drop_after - 1u : (set.pattern + 1u) % NPAT);
}

/* PLAY: the track from its first item (TRK_FROM; a DROP there: the pattern after it), from bar 1; HOME (home: 0, from
 * TRACKS: it stays) */
static void track_play(int home)
{
    const track_t *t = trk_now();
    int i = trk_pat_from(t, TRK_FROM < trk_len(t) ? TRK_FROM : 0u, 0);
    char n[8] = "TRACK ";
    if (i < 0) {
        app_say("TRACK", "EMPTY");
        return;
    }
    set.pattern = (uint8_t)(t->it[i] - 1u);
    pat_next = -1;
    pat_apply();
    trk_brk_go((uint32_t)i);             /* (its break) */
    if (playing_now())
        transport(1, 1);                 /* (from bar 1) */
    else {
        brk_reset();
        transport(1, 0);
    }
    ui.trk_on = 2u;
    ui.trk_pos = (uint8_t)i;
    ui.trk_loops = ui.trk_cnt = ui.trk_stop = 0u;
    ui.trk_drop = 0xFFu;
    ui.trk_prev = -1;
    if (home)
        ui.page = PG_HOME;
    ui.redraw = RG_ALL;
    int_text((int32_t)set.trk + 1, n + 6);
    app_say(n, "PLAYING");
}
static void track_stop(const char *why)
{
    ui.trk_on = 0u;
    ui.trk_drop = 0xFFu;
    app_say("TRACK", why);
    ui.redraw |= RG_HEAD | RG_BLOCKS;
}
/* the track's next item, as the last bar of the one playing begins (its last loop) */
static void track_next(void)
{
    const track_t *t = trk_now();
    uint32_t n = trk_len(t), i = ui.trk_pos + 1u;
    while (i < n && t->it[i] == TI_GAP)  /* (a gap left: played past) */
        i++;
    if (i >= n) {                        /* its end */
        if (t->end == 1u) {
            ui.trk_stop = 1u;            /* (at the loop's end) */
            return;
        }
        if (t->end == 2u) {
            track_stop("END  HELD");
            return;
        }
        i = 0u;
        while (i < n && t->it[i] == TI_GAP)
            i++;
        if (i >= n) {
            track_stop("EMPTY");
            return;
        }
    }
    if (t->it[i] == TI_DROP) {           /* a drop: the build, into the pattern after it */
        int j = trk_pat_from(t, i + 1u, t->end == 0u);
        if (j < 0) {
            ui.trk_pos = (uint8_t)i;
            return;
        }
        ui.trk_drop = (uint8_t)j;
        ui.trk_pos = (uint8_t)i;
        drop_start((uint32_t)t->it[j] - 1u);
        return;
    }
    ui.trk_pos = (uint8_t)i;
    ui.trk_loops = 0u;
    if (t->it[i] - 1u != set.pattern)
        pat_next = (int8_t)(t->it[i] - 1u);
    trk_brk_go(i);                       /* (its break, from the same bar: break_poll swaps at its last 16th) */
    ui.redraw |= RG_HEAD | RG_BLOCKS;
}
/* the main loop, each frame: the loops counted (one as its last bar begins), the next item asked for */
static void track_poll(const brk_status_t *st)
{
    if (!ui.trk_on)
        return;
    if (!st->playing) {
        if (ui.trk_on == 1u)
            track_stop("STOPPED");
        return;
    }
    ui.trk_on = 1u;
    if (st->step < 0)
        return;
    if (st->step < ui.trk_prev) {        /* a new loop */
        ui.trk_cnt = 0u;
        if (ui.trk_stop) {
            transport(0, 0);
            track_stop("END");
            return;
        }
    }
    ui.trk_prev = (int16_t)st->step;
    if (build_p >= 0.0f || build_wait || ui.trk_drop != 0xFFu)
        return;                          /* (a drop on its way: the track waits for it) */
    {
        uint32_t len = pat_len(), last = len > 16u ? len - 16u : 0u;
        const track_t *t = trk_now();
        if ((uint32_t)st->step >= last && !ui.trk_cnt) {
            ui.trk_cnt = 1u;
            if (t->it[ui.trk_pos % TRK_ITEMS] == TI_DROP || ++ui.trk_loops >= t->rep[ui.trk_pos % TRK_ITEMS])
                track_next();
            ui.redraw |= RG_HEAD | (ui.page == PG_TRACK ? RG_BLOCKS : 0);
        }
    }
}
/* the break a drop brings in, + 1 (0: none): a track's, the pattern dropped into's own; else DROP's DROP BREAK */
static uint32_t trk_drop_brk(void)
{
    uint32_t b = ui.trk_on && ui.trk_drop != 0xFFu ? *trk_brk(ui.trk_drop) : 0u;
    if (!b || b > n_breaks)
        b = set.drop_brk <= n_breaks ? set.drop_brk : 0u;
    return b;
}
/* the drop came (the main loop, drop_n): a track's on from the pattern it went to */
static void track_dropped(void)
{
    if (ui.trk_drop == 0xFFu)
        return;
    ui.trk_pos = ui.trk_drop;
    ui.trk_drop = 0xFFu;
    ui.trk_loops = 0u;
    ui.trk_cnt = 0u;
    ui.trk_prev = -1;
}

/* ---- KNOB: the SEL + knob functions, each its settings (K1 its amount, as SEL + its knob). Functions: 0 DUCK, 1 TERROR
 * 2, 2 COMP, 3 TERROR, 4 TERROR + (TERROR's more: no amount of its own), 5 DRIVE (the kick's INTENSITY). SEL + K1 is
 * DUCK, SEL + K3 COMP; SEL + K2 and SEL + K4 each the function KNOB gives them (LOAF BREAKER TEST: set.selk_asg) ---- */
/* LOAF BREAKER (9 Nov 2026): 9 TUNE (the kick's, SYNTH's), 10 DECAY (the SUB's), 11 REV BASS (its amount, start,
 * curve), 12 GLIDE, 13 CLEAN SUB (the round sine under the distortion), 14 MORPH (towards another kick memory) */
#define SK_N 15u
static const char *const SK_NAME[SK_N] = {"DUCK", "TERROR 2", "COMP", "TERROR", "TERROR +", "DRIVE", "T+ TYPE",
                                          "T+ SCREECH", "T+ TAIL", "TUNE", "DECAY", "REV BASS", "GLIDE", "CLEAN SUB",
                                          "MORPH"};
static const char *const SK_SHORT[SK_N] = {"DUCK", "TERR 2", "COMP", "TERROR", "T+ DRIVE", "DRIVE", "T+ TYPE", "SCRCH",
                                           "T+ TAIL", "TUNE", "DECAY", "REV B", "GLIDE", "C SUB", "MORPH"};
static const char *const SK_KNOB[SK_N][4] = {{"AMOUNT", "ATTACK", "RELEASE", "DUCKS"}, {"AMOUNT", "SUB", "SCREAM", "PUSH"},
                                             {"AMOUNT", "RATIO", "ATTACK", "RELEASE"}, {"AMOUNT", "SUB", "FIZZ", "SUSTAIN"},
                                             {"DRIVE", "TYPE", "SCREECH", "TAIL"}, {"AMOUNT", "", "", ""},
                                             {"TYPE", "", "", ""}, {"SCREECH", "", "", ""}, {"TAIL", "", "", ""},
                                             {"SEMITONES", "", "", ""}, {"SUB DECAY", "", "", ""},
                                             {"AMOUNT", "START", "CURVE", ""}, {"TIME", "", "", ""}, {"AMOUNT", "", "", ""},
                                             {"AMOUNT", "TO KICK", "", ""}};
static const char *const T_TYPE_NAME[5] = {"FOLD", "CLIP", "CRUSH", "RECT", "TUBE"};
/* what a SEL + knob can be given (KNOB, PRESETS), in that order: its code 1..15 is its place + 1 (0: the knob's own) */
#define SK_CODES 15u
static const uint8_t SK_CODE[SK_CODES] = {0u, 1u, 2u, 3u, 4u, 6u, 7u, 8u, 5u, 9u, 10u, 11u, 12u, 13u, 14u};
static const uint8_t SK_OWN[4] = {5u, 1u, 7u, 3u};   /* K1 DRIVE, K2 TERROR 2, K3 T+ SCREECH, K4 TERROR (LOAF BREAKER:
                                          * DUCK and COMP no more: the MIXER's MASTER has them) */
static const char *sk_short(uint32_t f) { return SK_SHORT[f % SK_N]; }   /* (EDIT KICK's KNOBS) */
static const char *sk_kname(uint32_t f, int i) { return SK_KNOB[f % SK_N][i & 3]; }
static int sk_live(uint32_t f) { return f != 0u && f != 2u; }   /* (a function a SEL + knob may have) */
static uint32_t sk_code(uint32_t knob)   /* the knob's code, 0 its own */
{
    uint8_t b = knob == 1u || knob == 3u ? set.selk_asg : set.selk_asg2;
    return knob >= 2u ? b >> 4 : b & 15u;
}
static uint32_t selk_fn(uint32_t knob)   /* the function SEL + K1..K4 turns */
{
    uint32_t c = sk_code(knob & 3u);
    return c && c <= SK_CODES && sk_live(SK_CODE[c - 1u]) ? SK_CODE[c - 1u] : SK_OWN[knob & 3u];
}
static uint8_t *sk_byte(uint32_t f, int i)   /* a setting's byte (the TERRORs': the kick's, int8 -50..50) */
{
    kick_t *k = &set.kick;
    static uint8_t *const DUCKB[3] = {&set.duck, &set.duck_att, &set.duck_rel};
    static uint8_t *const COMPB[4] = {&set.comp, &set.comp_ratio, &set.comp_att, &set.comp_rel};
    if (f == 0u)
        return i < 3 ? DUCKB[i] : &set.duck_what;
    if (f == 2u)
        return COMPB[i & 3];
    if (f == 4u)
        return i == 0 ? &KICK_T_DRIVE(k) : i == 1 ? &KICK_T_TYPE(k) : i == 2 ? &KICK_T_SCREECH(k) : &KICK_T_TAIL(k);
    if (i == 0)
        return kick_hc(k, f == 1u ? 2 : 1);
    if (f == 1u)
        return i == 1 ? &KICK_T2_SUB(k) : i == 2 ? &KICK_T2_SCREAM(k) : &KICK_T2_PUSH(k);
    return i == 1 ? &KICK_T_SUB(k) : i == 2 ? &KICK_T_FIZZ(k) : &KICK_T_SUS(k);
}
static int sk_signed(uint32_t f, int i)  /* a setting -50 .. +50 (int8) */
{
    return (f == 1u || f == 3u) ? i > 0 : f == 4u && (i == 0 || i == 3);
}
static void selk_value(uint32_t f, int i, char *t)
{
    uint8_t v;
    int32_t sv;
    t[0] = 0;
    if (f >= 9u) {                       /* LOAF BREAKER: TUNE, DECAY, REV BASS, GLIDE, CLEAN SUB, MORPH */
        int32_t x = f == 9u ? set.ksyn[KS_TUNE] : 0;
        if (f == 9u && i == 0) {
            if (x > 0)
                text_cat(t, "+");
            int_text(x, t + (x > 0 ? 1 : 0));
            text_cat(t, " ST");
        } else if (f == 10u && i == 0) {
            int_text((int32_t)(kick_decay_ms(KL_SUB, set.kick.l[KL_SUB].decay) + 0.5f), t);
            text_cat(t, " MS");
        } else if (f == 11u && i < 3) {
            int_text(set.kbass[1 + i], t);
            text_cat(t, "%");
        } else if (f == 12u && i == 0) {
            int_text((int32_t)((float)set.kbass[0] * (float)set.kbass[0] * 0.04f + 0.5f), t);
            text_cat(t, " MS");
        } else if (f == 13u && i == 0) {
            int_text(set.csub, t);
            text_cat(t, "%");
        } else if (f == 14u && i == 0) {
            int_text(set.kmorph, t);
            text_cat(t, "%");
        } else if (f == 14u && i == 1) {
            text_cat(t, "KICK ");
            int_text((int32_t)(set.kmorph_to % KICK_MEMS) + 1, t + 5);
        }
        return;
    }
    if (f >= 6u) {                       /* (T+ TYPE, SCREECH, TAIL alone: TERROR +'s) */
        if (i == 0)
            selk_value(4u, (int)(f - 5u), t);
        return;
    }
    if (f == 5u) {                       /* DRIVE: the kick's INTENSITY */
        if (i == 0)
            pct_text(set.kick.intensity, t);
        return;
    }
    v = *sk_byte(f, i);
    sv = (int8_t)v;
    if (f == 4u && i == 1)
        text_cat(t, T_TYPE_NAME[v <= 4u ? v : 0u]);
    else if (f == 4u && i == 2) {        /* SCREECH: its band */
        if (!v)
            text_cat(t, "OFF");
        else {
            int_text((int32_t)(500.0f * brk_exp2f_c((float)v * 0.0332f) + 0.5f), t);
            text_cat(t, " HZ");
        }
    } else if (sk_signed(f, i) && !(i == 1 || (f == 1u && i == 2))) {
        if (sv > 0)
            text_cat(t, "+");
        int_text(sv, t + (sv > 0 ? 1 : 0));
    } else if (i == 0) {                 /* the amount */
        if (!v)
            text_cat(t, "OFF");
        else if (f == 2u) {              /* COMP: its threshold */
            tenths_text(-(int32_t)v * 36 / 10, t);
            text_cat(t, " DB");
        } else {
            int_text(v, t);
            text_cat(t, "%");
        }
    } else if (f == 0u)
        if (i == 3)
            text_cat(t, v == 1u ? "+ DRUMS" : "BREAK");
        else
            ms_text(i == 1 ? att_ms(v) : rel_ms(v), t);
    else if (f == 2u) {
        if (i == 1) {
            float r = COMP_RATIO[v & 7u];
            if (r < 2.0f)
                text_cat(t, "1.5");
            else
                int_text((int32_t)r, t);
            text_cat(t, ":1");
        } else
            ms_text(i == 2 ? att_ms(v) : rel_ms(v), t);
    } else {                             /* SUB, SCREAM: about their frequency */
        int_text((int32_t)((i == 1 ? 110.0f * brk_exp2f_c((float)sv * 0.024f) : 620.0f * brk_exp2f_c((float)sv * 0.04f)) + 0.5f), t);
        text_cat(t, " HZ");
    }
}
static float sk_norm(uint32_t f, int i)  /* a setting 0..1, as its bar */
{
    uint8_t v;
    if (f >= 9u)                         /* (LOAF BREAKER: TUNE .. MORPH) */
        return f == 9u ? (i ? 0.0f : ((float)set.ksyn[KS_TUNE] + 12.0f) * (1.0f / 24.0f)) :
               f == 10u ? (i ? 0.0f : set.kick.l[KL_SUB].decay) :
               f == 11u ? (i < 3 ? (float)set.kbass[1 + i] * 0.01f : 0.0f) :
               f == 12u ? (i ? 0.0f : (float)set.kbass[0] * 0.01f) :
               f == 13u ? (i ? 0.0f : (float)set.csub * 0.01f) :
               i == 0 ? (float)set.kmorph * 0.01f : i == 1 ? (float)(set.kmorph_to % KICK_MEMS) * (1.0f / 9.0f) : 0.0f;
    if (f >= 6u)
        return i == 0 ? sk_norm(4u, (int)(f - 5u)) : 0.0f;
    if (f == 5u)
        return i == 0 ? set.kick.intensity : 0.0f;
    v = *sk_byte(f, i);
    if (sk_signed(f, i))
        return ((float)(int8_t)v + 50.0f) * 0.01f;
    if (f == 4u && i == 1)
        return (float)v * 0.25f;
    if ((f == 0u && i == 3))
        return (float)v;
    if (f == 2u && i == 1)
        return (float)(v & 7u) * (1.0f / 7.0f);
    return (float)v * 0.01f;
}
/* turned: SEL + its knob (i 0), on KNOB, or on EDIT KICK's TERROR parts */
static void selk_turn(uint32_t f, int i, int32_t d)
{
    char t[16], n[20];
    if (f >= 9u) {                       /* LOAF BREAKER: TUNE, DECAY, REV BASS, GLIDE, CLEAN SUB, MORPH */
        if (f == 9u && i == 0)
            set.ksyn[KS_TUNE] = (int8_t)clamp((int32_t)set.ksyn[KS_TUNE] + (d > 0 ? 1 : -1), -12, 12);
        else if (f == 10u && i == 0)
            set.kick.l[KL_SUB].decay = knob_f(set.kick.l[KL_SUB].decay, d, 0.02f, 0.0f, 1.0f);
        else if (f == 11u && i < 3)
            set.kbass[1 + i] = (uint8_t)clamp((int32_t)set.kbass[1 + i] + d * 2, 0, 100);
        else if (f == 12u && i == 0)
            set.kbass[0] = (uint8_t)clamp((int32_t)set.kbass[0] + d * 2, 0, 100);
        else if (f == 13u && i == 0)
            set.csub = (uint8_t)clamp((int32_t)set.csub + d * 2, 0, 100);
        else if (f == 14u && i == 0)
            set.kmorph = (uint8_t)clamp((int32_t)set.kmorph + d * 2, 0, 100);
        else if (f == 14u && i == 1)
            set.kmorph_to = (uint8_t)clamp((int32_t)(set.kmorph_to % KICK_MEMS) + (d > 0 ? 1 : -1), 0, KICK_MEMS - 1);
        else
            return;
    } else if (f >= 6u) {                /* (T+ TYPE, SCREECH, TAIL alone) */
        if (i == 0)
            selk_turn(4u, (int)(f - 5u), d);
        return;
    } else if (f == 5u) {                /* DRIVE */
        if (i)
            return;
        set.kick.intensity = knob_f(set.kick.intensity, d, 0.02f, 0.0f, 1.0f);
    } else {
        uint8_t *b = sk_byte(f, i);
        if (sk_signed(f, i))
            *b = (uint8_t)(int8_t)clamp((int32_t)(int8_t)*b + d * 2, -50, 50);
        else if (f == 4u && i == 1)
            *b = (uint8_t)clamp((int32_t)*b + (d > 0 ? 1 : -1), 0, 4);   /* (LOAF BREAKER: RECT, TUBE too) */
        else if (f == 0u && i == 3)
            *b = (uint8_t)clamp((int32_t)*b + (d > 0 ? 1 : -1), 0, 1);
        else if (f == 2u && i == 1)
            *b = (uint8_t)clamp((int32_t)*b + (d > 0 ? 1 : -1), 0, 7);
        else
            *b = (uint8_t)clamp((int32_t)*b + d * 2, 0, 100);
    }
    selk_value(f, i, t);
    n[0] = 0;
    text_cat(n, SK_NAME[f % SK_N]);
    if (i || f == 4u) {
        text_cat(n, " ");
        text_cat(n, SK_KNOB[f % SK_N][i & 3]);
    }
    app_say(n, t);
    changed();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* PRESETS on KNOB: what SEL + the knob picked does (DUCK, TERROR 2, COMP, TERROR, T+ DRIVE, T+ TYPE, T+ SCREECH,
 * T+ TAIL, DRIVE), any of the four */
static void selk_assign_k(uint32_t kn, int32_t d)   /* (ENV KNOB's knob, or EDIT KICK's K SET's) */
{
    uint32_t f, c = 0u, k;
    uint8_t *b;
    uint32_t sh;
    char n[12] = "SEL + K";
    kn &= 3u;
    f = selk_fn(kn);
    sh = kn >= 2u ? 4u : 0u;
    b = kn == 1u || kn == 3u ? &set.selk_asg : &set.selk_asg2;
    for (k = 0; k < SK_CODES; k++)       /* (where it is in the list now) */
        if (SK_CODE[k] == f)
            c = k;
    do                                   /* (DUCK and COMP passed over) */
        c = (uint32_t)clamp((int32_t)c + (d > 0 ? 1 : -1), 0, (int32_t)SK_CODES - 1);
    while (!sk_live(SK_CODE[c]) && c > 0u && c < SK_CODES - 1u);
    if (!sk_live(SK_CODE[c]))
        c = c ? SK_CODES - 1u : 1u;
    *b = (uint8_t)((*b & ~(15u << sh)) | (c + 1u) << sh);
    n[7] = (char)('1' + kn);
    n[8] = 0;
    app_say(n, SK_NAME[selk_fn(kn)]);
    changed();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}

/* ---- the page ---- */
static void trk_item_text(const track_t *t, uint32_t i, char *s, char *r)   /* "P3" "X4", "DROP" "", "END" "" */
{
    uint32_t v = t->it[i];
    s[0] = r[0] = 0;
    if (v == TI_END)
        text_cat(s, "END");
    else if (v == TI_DROP)
        text_cat(s, "DROP");
    else if (v == TI_GAP)
        text_cat(s, "EMPTY");
    else {
        s[0] = 'P';
        int_text((int32_t)v, s + 1);
        r[0] = 'X';
        int_text((int32_t)t->rep[i], r + 1);
    }
}
static void track_value(int i, char *t)
{
    const track_t *tr = trk_now();
    char r[6];
    t[0] = 0;
    if (RST_ON) {                        /* RESET (GLO): what, and K2-K4: the clicks */
        if (i == 0)
            text_cat(t, RST_NAME[ui.rst_sel % NRST]);
        else
            text_cat(t, rst_live() ? "MORE!" : "TURN x5");
        return;
    }
    if (ui.tpage == 4u) {                /* BUILD: its style, its filter, its noise, the gap before the drop */
        if (i == 0)
            text_cat(t, DS_NAME[set.drop_style % DS_N]);
        else if (i == 1)
            text_cat(t, SWEEP_NAME[set.drop_sweep & 3u]);
        else if (i == 2) {
            if (!set.drop_riser)
                text_cat(t, "OFF");
            else {
                int_text(set.drop_riser * 2u, t);
                text_cat(t, "%");
            }
        } else {
            static const char *const G[5] = {"NONE", "1/16", "1/8", "1 BEAT", "1/2 BAR"};
            text_cat(t, G[set.drop_gap % 5u]);
        }
        return;
    }
    if (ui.tpage == 2u) {                /* DROP */
        if (i == 0) {
            uint32_t b = DROP_BARS[set.drop_bars % 5u];
            if (!b)
                text_cat(t, "NONE");
            else {
                int_text((int32_t)b, t);
                text_cat(t, b > 1u ? " BARS" : " BAR");
            }
        } else if (i == 1) {
            if (!set.drop_after)
                text_cat(t, "NEXT");
            else {
                t[0] = 'P';
                int_text(set.drop_after, t + 1);
            }
        } else if (i == 2) {
            if (!set.drop_brk || set.drop_brk > n_breaks)
                text_cat(t, "SAME");
            else
                text_cat(t, break_name(set.drop_brk - 1u));
        } else if (set.drop_crash) {
            int_text(set.drop_crash, t);
            text_cat(t, "%");
        } else
            text_cat(t, "OFF");
        return;
    }
    if (ui.tpage == 1u) {                /* PLAY */
        if (i == 0) {
            text_cat(t, "TRACK ");
            int_text((int32_t)set.trk + 1, t + 6);
        } else if (i == 1)
            int_text((int32_t)TRK_FROM + 1, t);
        else if (i == 2)
            text_cat(t, TRK_END_NAME[tr->end % 3u]);
        return;
    }
    if (i == 0)                          /* TRACKS */
        int_text((int32_t)ui.trk_cur + 1, t);
    else if (i == 1)
        trk_item_text(tr, ui.trk_cur % TRK_ITEMS, t, r);
    else if (i == 2) {
        if (tr->it[ui.trk_cur % TRK_ITEMS] != TI_END && tr->it[ui.trk_cur % TRK_ITEMS] != TI_DROP &&
            tr->it[ui.trk_cur % TRK_ITEMS] != TI_GAP) {
            text_cat(t, "X");
            int_text(tr->rep[ui.trk_cur % TRK_ITEMS], t + 1);
        } else
            text_cat(t, "-");
    } else if (!trk_is_pat(tr, ui.trk_cur))
        text_cat(t, "-");
    else if (!*trk_brk(ui.trk_cur) || *trk_brk(ui.trk_cur) > n_breaks)
        text_cat(t, "SAME");
    else
        text_cat(t, break_name(*trk_brk(ui.trk_cur) - 1u));
}
static void track_say(int i)             /* the knob turned, on the strip */
{
    static const char *const N[NTPART][4] = {{"ITEM", "PUT", "LOOPS", "BREAK"}, {"PLAY", "FROM ITEM", "AT THE END", ""},
                                        {"BUILD", "AFTER DROP", "DROP BREAK", "CRASH"}, {"", "", "", ""},
                                        {"BUILD STYLE", "SWEEP", "RISER", "GAP"}, {"RESET", "RESET", "RESET", "RESET"}};
    char t[20];
    track_value(i, t);
    app_say(N[ui.tpage % NTPART][i & 3], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void track_turn(int i, int32_t d)
{
    track_t *t = trk_now();
    uint32_t c = ui.trk_cur % TRK_ITEMS;
    int dir = d > 0 ? 1 : -1;
    if (RST_ON) {                        /* RESET (GLO): K1 (or ALGORITHM) what; K2, K3 or K4 turned RST_TURNS clicks:
                                          * done */
        if (i == 0)
            reset_pick(d);
        else {
            uint32_t n = (uint32_t)(d < 0 ? -d : d);
            if (!rst_live())
                ui.rst_n = 0u;
            ui.rst_n = (uint8_t)(ui.rst_n + n < RST_TURNS ? ui.rst_n + n : RST_TURNS);
            ui.rst_arm = plat_ms() | 1u;
            if (ui.rst_n >= RST_TURNS) {
                reset_do(ui.rst_sel % NRST);
                ui.rst_n = 0u;
                ui.rst_arm = 0u;
                app_say(RST_NAME[ui.rst_sel % NRST], "RESET: DONE");
            } else {
                char t[16] = "TURN: ";
                int_text((int32_t)ui.rst_n, t + 6);
                text_cat(t, " / 5");
                app_say(RST_NAME[ui.rst_sel % NRST], t);
            }
        }
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    }
    if (ui.tpage == 4u) {                /* BUILD */
        if (i == 0)
            set.drop_style = (uint8_t)clamp((int32_t)set.drop_style + dir, 0, DS_N - 1);
        else if (i == 1)
            set.drop_sweep = (uint8_t)clamp((int32_t)set.drop_sweep + dir, 0, 3);
        else if (i == 2)
            set.drop_riser = (uint8_t)clamp((int32_t)set.drop_riser + d * 5, 0, 100);
        else
            set.drop_gap = (uint8_t)clamp((int32_t)set.drop_gap + dir, 0, 4);
        changed();
        track_say(i);
        return;
    }
    if (ui.tpage == 2u) {
        if (i == 0)
            set.drop_bars = (uint8_t)clamp((int32_t)set.drop_bars + dir, 0, 4);
        else if (i == 1)
            set.drop_after = (uint8_t)clamp((int32_t)set.drop_after + dir, 0, (int32_t)NPAT);
        else if (i == 2)
            set.drop_brk = (uint8_t)clamp((int32_t)set.drop_brk + dir, 0, (int32_t)n_breaks);
        else
            set.drop_crash = (uint8_t)clamp((int32_t)set.drop_crash + d * 5, 0, 100);
        changed();
    } else if (ui.tpage == 1u) {
        if (i == 0) {
            set.trk = (uint8_t)clamp((int32_t)set.trk + dir, 0, (int32_t)NTRACKS - 1);
            changed();
        } else if (i == 1) {
            uint32_t n = trk_len(t);
            TRK_FROM = (uint8_t)clamp((int32_t)TRK_FROM + dir, 0, n ? (int32_t)n - 1 : 0);
            set_dirty = 1u;
        } else if (i == 2) {
            t->end = (uint8_t)clamp((int32_t)t->end + dir, 0, 2);
            set_dirty = 1u;
        } else
            return;
    } else {
        if (i == 0) {                    /* the item: up to the one after the last */
            uint32_t n = trk_len(t);
            ui.trk_cur = (uint8_t)clamp((int32_t)c + dir, 0, (int32_t)(n < TRK_ITEMS ? n : TRK_ITEMS - 1u));
        } else if (i == 1) {             /* END, EMPTY, P1..P16, DROP (LOAF BREAKER: EMPTY left of P1, a case
                                          * emptied: gone with one more turn left, or ENV held) */
            int32_t v = t->it[c] == TI_DROP ? (int32_t)NPAT + 2 : t->it[c] == TI_GAP ? 1 :
                        t->it[c] == TI_END ? 0 : (int32_t)t->it[c] + 1;
            v = clamp(v + (t->it[c] == TI_END && dir > 0 ? 2 : dir), 0, (int32_t)NPAT + 2);   /* (the END: P1 at once) */
            if (v == 0 && trk_len(t) > c + 1u) {   /* (an END inside: the items after it moved up, the item gone) */
                uint32_t k;
                for (k = c; k + 1u < TRK_ITEMS; k++) {
                    t->it[k] = t->it[k + 1u];
                    t->rep[k] = t->rep[k + 1u];
                    *trk_brk(k) = *trk_brk(k + 1u);
                }
                t->it[TRK_ITEMS - 1u] = TI_END;
                *trk_brk(TRK_ITEMS - 1u) = 0u;
                if (TRK_FROM > c)        /* (the first item: where it was) */
                    TRK_FROM--;
                app_say("ITEM", "DELETED");
                set_dirty = 1u;
                ui.redraw |= RG_BLOCKS | RG_FOOT;
                return;
            }
            if (t->it[c] == TI_END && v && c > trk_len(t))
                return;
            t->it[c] = v == (int32_t)NPAT + 2 ? TI_DROP : v == 1 ? TI_GAP : v == 0 ? TI_END : (uint8_t)(v - 1);
            if (!t->rep[c])
                t->rep[c] = 1u;
        } else if (i == 2) {
            if (t->it[c] == TI_END || t->it[c] == TI_DROP || t->it[c] == TI_GAP)
                return;
            t->rep[c] = (uint8_t)clamp((int32_t)t->rep[c] + dir, 1, 16);
        } else {                         /* (LOAF BREAKER: K4 the item's break; the END on PLAY's K3) */
            if (!trk_is_pat(t, c))
                return;
            *trk_brk(c) = (uint8_t)clamp((int32_t)*trk_brk(c) + dir, 0, (int32_t)n_breaks);
        }
        set_dirty = 1u;
    }
    track_say(i);
}
/* PRESETS on ENV: the track (TRACKS and PLAY the same) */
static void track_pick(int32_t d)
{
    set.trk = (uint8_t)clamp((int32_t)set.trk + (d > 0 ? 1 : -1), 0, (int32_t)NTRACKS - 1);
    ui.trk_cur = 0u;
    changed();
    say_int("TRACK", (int32_t)set.trk + 1, "");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* LOAF BREAKER: TRACKS, SEL + PLAY: the item edited is where PLAY starts the track (again on it: from the first) */
static void track_from_here(void)
{
    const track_t *t = trk_now();
    uint32_t c = ui.trk_cur % TRK_ITEMS;
    if (c >= trk_len(t)) {
        app_say("START", "PAST THE END");
        return;
    }
    TRK_FROM = (uint8_t)(TRK_FROM == c ? 0u : c);
    set_dirty = 1u;
    say_int("START AT ITEM", (int32_t)TRK_FROM + 1, "");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* a white key on ENV. TRACKS: that pattern put at the item edited (its loops kept), then the next item; PLAY: that
 * track played (keys 1-8) */
static void track_key(uint32_t k)
{
    if (ui.tpage == 1u) {
        if (k < NTRACKS) {
            set.trk = (uint8_t)k;
            changed();
            track_play(1);
        }
        return;
    }
    if (ui.tpage == 0u) {
        track_t *t = trk_now();
        uint32_t c = ui.trk_cur % TRK_ITEMS;
        char s[8], r[6];
        if (c > trk_len(t))
            c = trk_len(t);
        if (t->it[c] == TI_END || t->it[c] == TI_DROP || t->it[c] == TI_GAP || !t->rep[c])
            t->rep[c] = 1u;
        t->it[c] = (uint8_t)(k + 1u);
        trk_item_text(t, c, s, r);
        say_int("ITEM", (int32_t)c + 1, "");
        app_say(ui.msg[0], s);
        if (c + 1u < TRK_ITEMS)
            ui.trk_cur = (uint8_t)(c + 1u);
        set_dirty = 1u;
        ui.redraw |= RG_BLOCKS | RG_FOOT;
    }
}
/* ENV held on TRACKS: the items after the one edited one place right, a gap after it (the one edited stays where it
 * is); the gap edited, for a white key (a pattern) or K2 (a DROP; K2 left: the gap gone again). Played past if left.
 * ENV held on a gap: it goes, the items on its sides closed up */
static void track_gap(void)
{
    track_t *t = trk_now();
    uint32_t n = trk_len(t), c = ui.trk_cur % TRK_ITEMS, k;
    if (c < n && t->it[c] == TI_GAP) {   /* (on a space: it goes, the items on each side close up, the cursor on the
                                          * one before) */
        for (k = c; k + 1u < TRK_ITEMS; k++) {
            t->it[k] = t->it[k + 1u];
            t->rep[k] = t->rep[k + 1u];
            *trk_brk(k) = *trk_brk(k + 1u);
        }
        t->it[TRK_ITEMS - 1u] = TI_END;
        *trk_brk(TRK_ITEMS - 1u) = 0u;
        if (ui.trk_on && ui.trk_pos > c)
            ui.trk_pos--;
        if (ui.trk_drop != 0xFFu && ui.trk_drop > c)
            ui.trk_drop--;
        if (TRK_FROM > c)
            TRK_FROM--;
        ui.trk_cur = (uint8_t)(c ? c - 1u : 0u);
        set_dirty = 1u;
        app_say("SPACE", "REMOVED");
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    }
    if (c >= n) {
        app_say("SPACE", "NOTHING AFTER");
        return;
    }
    if (n >= TRK_ITEMS) {
        app_say("SPACE", "TRACK FULL");
        return;
    }
    for (k = n; k > c + 1u; k--) {       /* (the END too, once there is room for it) */
        t->it[k] = t->it[k - 1u];
        t->rep[k] = t->rep[k - 1u];
        *trk_brk(k) = *trk_brk(k - 1u);
    }
    t->it[c + 1u] = TI_GAP;
    t->rep[c + 1u] = 0u;
    *trk_brk(c + 1u) = 0u;
    ui.trk_cur = (uint8_t)(c + 1u);
    if (ui.trk_on && ui.trk_pos > c && ui.trk_pos + 1u < TRK_ITEMS)
        ui.trk_pos++;                    /* (a track playing: on where it was) */
    if (ui.trk_drop != 0xFFu && ui.trk_drop > c && ui.trk_drop + 1u < TRK_ITEMS)
        ui.trk_drop++;
    if (TRK_FROM > c && TRK_FROM + 1u < TRK_ITEMS)
        TRK_FROM++;
    set_dirty = 1u;
    say_int("SPACE AT", (int32_t)c + 2, "");
    app_say(ui.msg[0], "KEY: A PATTERN");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static uint32_t track_keys_lit(void)     /* TRACKS: the item's pattern; PLAY: the tracks there are; KNOB: its function */
{
    uint32_t m = 0u, i;
    if (ui.tpage == 1u) {
        for (i = 0; i < NTRACKS; i++)
            if (trk_len(&pats.trk[i]))
                m |= 1u << i;
        return m;
    }
    if (ui.tpage == 0u) {
        uint32_t v = trk_now()->it[ui.trk_cur % TRK_ITEMS];
        return v && v <= NPAT ? 1u << (v - 1u) : 0u;
    }
    return 0u;
}

/* the picture, y 44..191 (148 rows, drawn in strips): TRACKS and PLAY the track's items in four rows of eight (a
 * pattern "P3" and its loops "X4", a DROP lit amber, the END), the item edited (TRACKS) or played from (PLAY)
 * outlined, the one playing lit; above them the track, its length and what it does at its end. DROP: its four
 * settings, large */
#define TK_GY 22                         /* the items' grid begins */
#define TK_CH 31                         /* a row of items */
/* LOAF BREAKER: ENV's PLAY, a radio: the stations (the tracks 1-8) on a lit dial, the needle gliding to the one K1
 * tunes, a big knob turned to it, the station's name and what it plays, PRESS PLAY blinking (ON AIR while it plays) and
 * the speaker's grille */
static void radio_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* a filled circle */
{
    int32_t y;
    for (y = -r; y <= r; y++) {
        int32_t w = 0;
        while ((w + 1) * (w + 1) + y * y <= r * r)
            w++;
        cv_rect(cx - w, cy + y, 2 * w + 1, 1, c);
    }
}
static void radio_picture(void)
{
    const track_t *t = trk_now();
    uint32_t i, n = trk_len(t), now = plat_ms();
    int32_t x, k;
    float a;
    char s[24], u[8];
    int on_air = ui.trk_on && 1;
    /* the dial: a lit window, a scale, the stations */
    cv_rect(6, 2, 228, 44, C_BLOCK);
    cv_rect(10, 6, 220, 36, RGB(40, 26, 12));
    for (x = 0; x <= 56; x++) {          /* (the scale's ticks, the long ones the stations) */
        int32_t px = 20 + x * 200 / 56, lng = !(x % 8);
        cv_rect(px, lng ? 22 : 26, 1, lng ? 12 : 5, lng ? C_AMB : RGB(120, 80, 40));
    }
    for (i = 0; i < NTRACKS; i++) {
        int32_t px = 20 + (int32_t)i * 200 / 7;
        s[0] = (char)('1' + i);
        s[1] = 0;
        cv_text(px - 2, 8, &FONT_XS, s, i == set.trk % NTRACKS ? C_WHITE : trk_len(&pats.trk[i]) ? C_AMB
                : RGB(110, 70, 35));
    }
    x = 20 + (int32_t)(ui.radio * 200.0f / 7.0f + 0.5f);
    cv_rect(x - 1, 7, 3, 34, C_HI);      /* the needle */
    cv_rect(x, 7, 1, 34, C_WHITE);
    /* the knob: a dark ring, its face, a line to where it points (-135..+135 degrees over the eight) */
    radio_disc(42, 100, 30, C_BLOCK);
    radio_disc(42, 100, 25, RGB(70, 70, 78));
    radio_disc(42, 100, 21, RGB(110, 110, 120));
    a = (-0.375f + 0.75f * ui.radio / 7.0f);   /* (turns from straight up) */
    for (k = 6; k <= 20; k++)
        cv_rect(42 + (int32_t)((float)k * brk_sin_turns(a)), 100 - (int32_t)((float)k * brk_sin_turns(a + 0.25f)), 3,
                3, C_HI);
    for (k = 0; k < 9; k++) {            /* (its marks around) */
        float b = -0.375f + 0.75f * (float)k / 8.0f;
        cv_rect(42 + (int32_t)(34.0f * brk_sin_turns(b)), 100 - (int32_t)(34.0f * brk_sin_turns(b + 0.25f)), 2, 2,
                C_GRAY);
    }
    cv_text(26, 136, &FONT_XS, "TUNING", C_GRAY);
    /* the station */
    s[0] = 0;
    text_cat(s, "TRACK ");
    int_text((int32_t)set.trk + 1, s + 6);
    cv_text(86, 56, &FONT_B, s, C_WHITE);
    s[0] = 0;
    int_text((int32_t)trk_bars(t), s);
    text_cat(s, " BARS  ");
    int_text((int32_t)n, u);
    text_cat(s, u);
    text_cat(s, n == 1u ? " ITEM" : " ITEMS");
    cv_text(86, 78, &FONT_XS, n ? s : "EMPTY: MAKE IT ON TRACKS", C_GRAY);
    s[0] = 0;
    text_cat(s, "FROM ");
    int_text((int32_t)TRK_FROM + 1, u);
    text_cat(s, u);
    text_cat(s, "  ");
    text_cat(s, TRK_END_NAME[t->end % 3u]);
    cv_text(86, 90, &FONT_XS, s, C_GRAY);
    if (on_air)                          /* ON AIR, or PRESS PLAY blinking */
        cv_text(86, 106, &FONT_T, (now / 400u) & 1u ? "ON AIR" : "", C_HI);
    else if (n)
        cv_text(86, 106, &FONT_T, (now / 500u) & 1u ? "PRESS PLAY" : "", C_HI);
    for (k = 0; k < 4; k++)              /* the speaker's grille */
        for (x = 0; x < 24; x++)
            cv_rect(88 + x * 6, 124 + k * 6, 2, 2, RGB(60, 60, 66));
}
static void track_picture(int32_t top)
{
    const track_t *t = trk_now();
    uint32_t i, n = trk_len(t);
    char s[20], r[8];
    cv_oy = -top;
    if (RST_ON) {                        /* RESET (GLO): the list (K1, ALGORITHM), what it puts back, the clicks */
        uint32_t w = ui.rst_sel % NRST;
        uint32_t on = rst_live() ? ui.rst_n : 0u;
        {                                /* (five at a time, the one picked kept in view) */
            uint32_t first = w < 2u ? 0u : w + 3u > NRST ? NRST - 5u : w - 2u;
            for (i = first; i < first + 5u && i < NRST; i++) {
                int32_t y = 6 + (int32_t)(i - first) * 20;
                cv_rect(6, y, 108, 17, i == w ? C_HI : C_BLOCK);
                cv_text(12, y + 2, &FONT_T, RST_NAME[i], i == w ? C_BLACK : C_GRAY);
            }
            if (first)                   /* (more above, more below) */
                cv_rect(116, 6, 2, 6, C_DIM);
            if (first + 5u < NRST)
                cv_rect(116, 92, 2, 6, C_DIM);
        }
        cv_text(124, 8, &FONT_XS, "BACK TO THE START:", C_GRAY);
        {                                /* (what it puts back, in lines of ~18) */
            const char *p = RST_WHAT[w];
            int32_t y = 22;
            while (*p && y < 100) {
                char line[20];
                uint32_t n = 0u, cut = 0u;
                while (p[n] && n < 18u) {
                    if (p[n] == ' ')
                        cut = n;
                    n++;
                }
                if (p[n] && cut)
                    n = cut;
                memcpy(line, p, n);
                line[n] = 0;
                cv_text(124, y, &FONT_XS, line, C_WHITE);
                p += n;
                while (*p == ' ' || *p == ',')
                    p++;
                y += 12;
            }
        }
        cv_rect(6, 104, 148, 24, on ? C_HI : C_BLOCK);
        cv_text(12, 110, &FONT_T, on ? "KEEP TURNING" : "TURN A KNOB x5", on ? C_BLACK : C_GRAY);
        for (i = 0; i < RST_TURNS; i++)  /* (the clicks so far: five boxes) */
            cv_rect(160 + (int32_t)i * 15, 106, 12, 20, i < on ? C_HI : C_BLOCK);
        cv_oy = 0;
        return;
    }
    if (ui.tpage == 4u) {                /* BUILD: its four settings, large */
        static const char *const N[4] = {"BUILD STYLE", "SWEEP", "RISER", "GAP BEFORE THE DROP"};
        for (i = 0; i < 4u; i++) {
            int32_t y = 6 + (int32_t)i * 34;
            cv_text(10, y, &FONT_XS, N[i], C_GRAY);
            track_value((int)i, s);
            cv_text(10, y + 11, &FONT_B, s, C_WHITE);
        }
        cv_text(134, 12, &FONT_XS, "RANDOM: EACH", C_HI);
        cv_text(134, 24, &FONT_XS, "BUILD PICKS ITS", C_HI);
        cv_text(134, 36, &FONT_XS, "OWN STYLE, SWEEP", C_HI);
        cv_text(134, 60, &FONT_XS, "STUTTER: THE", C_GRAY);
        cv_text(134, 72, &FONT_XS, "BREAK ROLLS INTO", C_GRAY);
        cv_text(134, 84, &FONT_XS, "THE DROP", C_GRAY);
        cv_text(134, 104, &FONT_XS, "PRESETS: TEMPO RISE", C_GRAY);   /* (TEST) */
        s[0] = 0;
        if (!set.drop_accel)
            text_cat(s, "OFF");
        else {
            s[0] = '+';
            int_text(set.drop_accel, s + 1);
            text_cat(s, "% TO THE DROP");
        }
        cv_text(134, 116, &FONT_XS, s, set.drop_accel ? C_HI : C_DIM);
        cv_oy = 0;
        return;
    }
    if (ui.tpage == 2u) {
        static const char *const N[4] = {"BUILD", "AFTER DROP", "DROP BREAK", "CRASH"};
        for (i = 0; i < 4u; i++) {
            int32_t y = 6 + (int32_t)i * 34;
            cv_text(10, y, &FONT_XS, N[i], C_GRAY);
            track_value((int)i, s);
            cv_text(10, y + 11, &FONT_B, s, C_WHITE);
            cv_rect(120, y + 6, 1, 20, C_LINE);
        }
        cv_text(132, 12, &FONT_XS, "SEL + ENV: THE DROP", C_HI);
        cv_text(132, 26, &FONT_XS, "FROM THE NEXT BAR", C_GRAY);
        cv_text(132, 46, &FONT_XS, "NONE: THE BAR", C_GRAY);
        cv_text(132, 58, &FONT_XS, "PLAYING ENDS IN IT", C_GRAY);
        cv_text(132, 78, &FONT_XS, "SEL + ENV BUILDING:", C_GRAY);
        cv_text(132, 90, &FONT_XS, "THE DROP NEXT BAR", C_GRAY);
        cv_oy = 0;
        return;
    }
    if (ui.tpage == 1u) {                /* PLAY: the radio */
        radio_picture();
        cv_oy = 0;
        return;
    }
    s[0] = 0;
    text_cat(s, "TRACK ");
    int_text((int32_t)set.trk + 1, s + 6);
    cv_text(6, 2, &FONT_B, s, C_WHITE);
    s[0] = 0;
    int_text((int32_t)trk_bars(t), s);
    text_cat(s, " BARS  ");
    text_cat(s, TRK_END_NAME[t->end % 3u]);
    cv_text(234 - text_w(&FONT_XS, s), 6, &FONT_XS, s, C_GRAY);
    if (ui.trk_on && ui.tpage == 1u)
        cv_text(100, 6, &FONT_XS, "PLAYING", C_HI);
    if (!ui.tpage && TRK_FROM && TRK_FROM < n) {   /* (LOAF BREAKER: where PLAY starts it) */
        char f[12] = "FROM ";
        int_text((int32_t)TRK_FROM + 1, f + 5);
        cv_text(100, 6, &FONT_XS, f, C_WHITE);
    }
    for (i = 0; i < TRK_ITEMS; i++) {
        int32_t x = 1 + (int32_t)(i % 8u) * 30, y = TK_GY + (int32_t)(i / 8u) * TK_CH;
        int mark = ui.tpage ? i == TRK_FROM : i == ui.trk_cur;
        int now = ui.trk_on && i == ui.trk_pos;
        uint16_t bg = now ? C_HI : i < n ? (t->it[i] == TI_DROP ? C_AMB : C_BLOCK) : C_BLACK;
        if (i > n)
            cv_rect(x + 10, y + 13, 8, 1, C_LINE);   /* (past the END: nothing) */
        else if (i < n && t->it[i] == TI_GAP) {   /* (a gap: its outline, a +) */
            cv_rect(x, y, 28, 1, C_LINE);
            cv_rect(x, y + TK_CH - 4, 28, 1, C_LINE);
            cv_rect(x, y, 1, TK_CH - 3, C_LINE);
            cv_rect(x + 27, y, 1, TK_CH - 3, C_LINE);
            cv_rect(x + 10, y + 12, 9, 1, C_GRAY);
            cv_rect(x + 14, y + 8, 1, 9, C_GRAY);
        } else
            cv_rect(x, y, 28, TK_CH - 3, bg);
        if (mark) {                      /* (the item edited: a white frame) */
            cv_rect(x - 1, y - 1, 30, 1, C_WHITE);
            cv_rect(x - 1, y + TK_CH - 3, 30, 1, C_WHITE);
            cv_rect(x - 1, y - 1, 1, TK_CH - 1, C_WHITE);
            cv_rect(x + 28, y - 1, 1, TK_CH - 1, C_WHITE);
        }
        if (i == TRK_FROM && i < n && !ui.tpage) {   /* (LOAF BREAKER: the first item PLAY plays: a flag at its corner) */
            uint16_t fc = now ? C_BLACK : C_WHITE;
            cv_rect(x + 1, y + 1, 1, 7, fc);
            cv_rect(x + 2, y + 2, 1, 5, fc);
            cv_rect(x + 3, y + 3, 1, 3, fc);
            cv_rect(x + 4, y + 4, 1, 1, fc);
        }
        if (i > n || (i < n && t->it[i] == TI_GAP))
            continue;
        trk_item_text(t, i, s, r);
        {
            uint16_t c = now || t->it[i] == TI_DROP ? C_BLACK : i == n ? C_DIM : C_WHITE;
            if (t->it[i] == TI_DROP)
                cv_text(x + (28 - text_w(&FONT_XS, "DRP")) / 2, y + 9, &FONT_XS, "DRP", c);
            else {
                cv_text(x + (28 - text_w(&FONT_T, s)) / 2, y + 3, &FONT_T, s, c);
                if (r[0] && *trk_brk(i) && *trk_brk(i) <= n_breaks) {   /* (its own break: "X4 B3") */
                    char b[4] = "B";
                    int_text((int32_t)*trk_brk(i), b + 1);
                    if (text_w(&FONT_XS, r) + text_w(&FONT_XS, b) + 3 <= 27) {
                        int32_t w = text_w(&FONT_XS, r) + 3 + text_w(&FONT_XS, b), x0 = x + (28 - w) / 2;
                        cv_text(x0, y + 16, &FONT_XS, r, now ? C_BLACK : C_GRAY);
                        cv_text(x0 + text_w(&FONT_XS, r) + 3, y + 16, &FONT_XS, b, now ? C_BLACK : C_AMB);
                    } else
                        cv_text(x + (28 - text_w(&FONT_XS, b)) / 2, y + 16, &FONT_XS, b, now ? C_BLACK : C_AMB);
                } else if (r[0])
                    cv_text(x + (28 - text_w(&FONT_XS, r)) / 2, y + 16, &FONT_XS, r, now ? C_BLACK : C_GRAY);
            }
        }
    }
    cv_oy = 0;
}
static void draw_track(void)
{
    int32_t top;
    for (top = 0; top < 148; top += 60) {   /* (the canvas holds 60 rows) */
        int32_t h = 148 - top < 60 ? 148 - top : 60;
        cv_begin(240, (uint32_t)h, C_BLACK);
        track_picture(top);
        cv_blit(0, (uint32_t)(44 + top));
    }
}
/* ENV: TRACKS; again, PLAY, DROP, BUILD, and back */
static void track_open(void)
{
    if (ui.page == PG_TRACK) {
        uint32_t k;
        for (k = 0; k < NTORD && TORDER[k] != ui.tpage; k++)
            ;
        ui.tpage = TORDER[(k + 1u) % NTORD];
    }
    else {
        ui.page = PG_TRACK;
        ui.tpage = 0u;
    }
    ui.redraw |= RG_FOOT | RG_BLOCKS;
    page_say();
}
