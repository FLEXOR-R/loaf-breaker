/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the KICK page (EDIT): the kick's layers, effects, macros and output. (Part of app.c's unity: included
 * there in order.) */

/* ---- the KICK page (PRD K-*): EDIT opens it, EDIT again the next sub-page. The white keys are the kick's 16
 * steps, ALGORITHM the layer, PRESETS the built-in kicks, SEL + EDIT through the FX chain or around it */
enum { KP_MIX, KP_LAYER, KP_FX, KP_MACRO, KP_OUT, KP_TERROR, KP_TMORE, KP_TERROR2, KP_SYN1, KP_SYN2, KP_BASS, KP_KNOBS,
       KP_KSET, KP_CMORPH, KP_STYLE, KPAGES };
/* (LOAF BREAKER TEST: TERROR, TERROR +, TERROR 2 the kick's own settings of them, ENV's KNOB functions 3, 4, 1) */
static void selk_turn(uint32_t f, int i, int32_t d);
static void selk_value(uint32_t f, int i, char *t);
static float sk_norm(uint32_t f, int i);
static uint32_t selk_fn(uint32_t knob);
static const char *sk_short(uint32_t f);
static const char *sk_kname(uint32_t f, int i);
/* LOAF BREAKER: the KNOBS section: KNOBS, K1-K4 what SEL + K1..K4 turns (each function's first setting), with the
 * kick's picture; K1 SET .. K4 SET (ui.kks), that knob's function's settings, as ENV KNOB. The kick playing's (each
 * kick memory its own: steps.kx) */
static int kp_knobs(void) { return ui.kpage == KP_KNOBS || ui.kpage == KP_KSET; }
static uint32_t kn_fn(int i) { return selk_fn(ui.kpage == KP_KSET ? ui.kks & 3u : (uint32_t)(i & 3)); }
static int kn_i(int i) { return ui.kpage == KP_KSET ? i : 0; }
/* LOAF BREAKER: SYNTH's CLEAN · MORPH part: K1 CLEAN SUB, K2 MORPH, K3 its TO KICK (ENV KNOB's functions 13, 14: their
 * own place, the kick's own, with no knob given them) */
static uint32_t cm_fn(int i) { return i == 0 ? 13u : 14u; }
/* LOAF BREAKER: KNOBS' STYLE part: K1-K4 each its knob's function (one at a time), PRESETS a style (TERROR, EARLY NEXT
 * GEN, HARDCORE ..) giving the four SEL + knobs their functions at once (the kick playing's); track.c has them */
static void kstyle_turn(int32_t d);
static void selk_assign_k(uint32_t kn, int32_t d);
static const char *kstyle_name(void);
static int cm_i(int i) { return i == 2 ? 1 : 0; }
static uint32_t kp_fn(void) { return ui.kpage == KP_TERROR ? 3u : ui.kpage == KP_TMORE ? 4u : 1u; }
static int kp_terror(void) { return ui.kpage >= KP_TERROR && ui.kpage <= KP_TERROR2; }
/* LOAF BREAKER: the kick's SYNTH (kick.h KS_*), its value as the foot shows it, as its bar, turned */
static uint32_t ks_of(int i) { return (ui.kpage == KP_SYN2 ? 4u : 0u) + (uint32_t)(i & 3); }
/* LOAF BREAKER TEST: the kick's BASS part: K1 GLIDE (0 .. 400 ms between two notes), K2 REV BASS (its level), K3 its
 * START (1/8 .. 5/8 of the beat), K4 its CURVE (slow .. sharp) */
static void kb_value(int i, char *t)
{
    uint32_t v = set.kbass[i & 3];
    t[0] = 0;
    if (i == 0 || i == 1) {
        if (!v) {
            text_cat(t, "OFF");
            return;
        }
        int_text(i == 0 ? (int32_t)(v * v * 4u / 100u) : (int32_t)v, t);
        text_cat(t, i == 0 ? " MS" : "%");
    } else if (i == 2) {
        int_text((int32_t)(125u + v * 5u) / 10, t);
        text_cat(t, "% BEAT");
    } else
        text_cat(t, v < 25u ? "SLOW" : v < 50u ? "SOFT" : v < 75u ? "SHARP" : "SNAP");
}
static void ks_text(uint32_t j, char *t);
static void ks_value(int i, char *t)
{
    if (ui.kpage == KP_BASS) {
        kb_value(i, t);
        return;
    }
    ks_text(ks_of(i), t);
}
static void ks_text(uint32_t j, char *t)   /* (LOAF BREAKER: a SYNTH setting's value, by its KS_*: SEL + knob's too) */
{
    int32_t v = set.ksyn[j];
    t[0] = 0;
    if (j == KS_TIME || j == KS_RTIME) {
        float ms = j == KS_TIME ? 120.0f * brk_exp2f_c((float)v * 0.04f) : 2048.0f * brk_exp2f_c((float)v * 0.02f) / 44.1f;
        int_text((int32_t)(ms + 0.5f), t);
        text_cat(t, " MS");
    } else if (j == KS_HOLD && v > 0) {
        int_text(v * 5, t);
        text_cat(t, " MS");
    } else if ((j == KS_RUMBLE || j == KS_ASYM || j == KS_HOLD) && v <= 0)
        text_cat(t, j == KS_HOLD && v < 0 ? "SHORTER" : "OFF");
    else if (j == KS_RUMBLE || j == KS_ASYM) {
        int_text(v * 2, t);
        text_cat(t, "%");
    } else {
        if (v > 0)
            text_cat(t, "+");
        int_text(v, t + (v > 0 ? 1 : 0));
        if (j == KS_TUNE)
            text_cat(t, " ST");
    }
}
static float ks_normj(uint32_t j);
static float ks_norm(int i)
{
    if (ui.kpage == KP_BASS)
        return (float)set.kbass[i & 3] * 0.01f;
    return ks_normj(ks_of(i));
}
static float ks_normj(uint32_t j)
{
    float v = (float)set.ksyn[j];
    return j == KS_TUNE ? (v + 12.0f) / 24.0f : j == KS_RUMBLE || j == KS_ASYM ? v / 50.0f : (v + 50.0f) * 0.01f;
}
static void ks_add(uint32_t j, int32_t d);
static void ks_turn(int i, int32_t d)
{
    if (ui.kpage == KP_BASS) {
        set.kbass[i & 3] = (uint8_t)clamp((int32_t)set.kbass[i & 3] + d * 2, 0, 100);
        return;
    }
    ks_add(ks_of(i), d);
}
static void ks_add(uint32_t j, int32_t d)   /* (a SYNTH setting turned, by its KS_*) */
{
    int32_t v = set.ksyn[j];
    v = j == KS_TUNE ? clamp(v + (d > 0 ? 1 : -1), -12, 12) : clamp(v + d * 2, j == KS_RUMBLE || j == KS_ASYM ? 0 : -50, 50);
    set.ksyn[j] = (int8_t)v;
}

/* LOAF BREAKER: the kick memories, KICK 1..10 (KICK's PRESETS, SEQ's SELECT): the one playing kept as it is, the next
 * as it was left (the volume kept), its knobs with it: ENV KNOB's choices, CLEAN SUB, BASS, MORPH (kx_put, kx_get) */
static void kick_mem_go(uint32_t n)      /* (LOAF BREAKER: KICK n, 0..9, as above; a track item's too) */
{
    float vol = set.kick.volume;
    char t[4];
    uint32_t j;
    set.kmem[set.kslot % KICK_MEMS] = set.kick;
    for (j = 0; j < KS_N; j++)           /* (LOAF BREAKER: its SYNTH with it) */
        set.ksmem[set.kslot % KICK_MEMS][j] = set.ksyn[j];
    kx_put(set.kslot);
    set.kslot = (uint8_t)(n % KICK_MEMS);
    set.kick = set.kmem[set.kslot];
    for (j = 0; j < KS_N; j++)
        set.ksyn[j] = set.ksmem[set.kslot][j];
    kx_get(set.kslot);
    set.kick.volume = vol;
    int_text((int32_t)set.kslot + 1, t);
    app_say("KICK", t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed_kick();
}
/* LOAF BREAKER: OUT's K4 PRESET: the built-in kicks (909 .. DISTORTO), then COPY KICK 1..10: that memory's kick, its
 * SYNTH and its knobs (KNOBS, CLEAN SUB, BASS, MORPH) onto the kick selected, its volume kept */
#define KPRE_N (KICK_PRESETS + KICK_MEMS)
static void kpre_text(char *t)
{
    uint32_t p = ui.kpreset % KPRE_N;
    if (p < KICK_PRESETS) {
        text_cat(t, KICK_PRESET_NAME[p]);
        return;
    }
    text_cat(t, "COPY K");
    int_text((int32_t)(p - KICK_PRESETS) + 1, t + 6);
}
static void kick_mem_turn(int32_t d)
{
    kick_mem_go((set.kslot % KICK_MEMS + (d > 0 ? 1u : KICK_MEMS - 1u)) % KICK_MEMS);
}
static const char *const KPAGE_NAME[KPAGES] = {"MIX", "LAYER", "LAYER FX", "MACRO", "OUT", "TERROR", "TERROR +",
                                                "TERROR 2", "SYNTH", "SYNTH +", "BASS", "KNOBS", "K SET",
                                                "CLEAN MORPH", "STYLE"};
/* the kick's parts in a row, as ALGORITHM turns through them: MIX, SUB, SUB FX, PUNCH, PUNCH FX, GRIT, GRIT FX,
 * CLICK, CLICK FX, MACRO, OUT (each a sub-page, and for a layer its layer); LOAF BREAKER TEST: then EQ BANDS, EQ
 * SHAPE (the KICK EQ page), FX 1, FX 2 (the FX page on the kick's slots), TERROR, TERROR +, TERROR 2 */
#define KP_BASE (3u + 2u * KICK_LAYERS)
#define KPARTS (KP_BASE + 17u)
static const char *const KP_MORE_NAME[11] = {"EQ BANDS", "EQ SHAPE", "FX 1", "FX 2", "TERROR", "TERROR +", "TERROR 2",
                                            "SYNTH", "SYNTH +", "BASS", "KNOBS"};   /* (then K1 SET .. K4 SET: "K1 C SUB") */   /* (SYNTH, SYNTH +: ALGORITHM left from MIX) */
static uint32_t kick_part(void)
{
    if (ui.page == PG_KEQ)
        return KP_BASE + (ui.keq_part & 1u);
    if (ui.page == PG_FX)
        return KP_BASE + 2u + (uint32_t)(ui.fx_focus - BRK_FX_SLOTS) % 2u;
    if (ui.kpage == KP_CMORPH)
        return KP_BASE + 15u;
    if (ui.kpage == KP_STYLE)
        return KP_BASE + 16u;
    if (kp_knobs())
        return KP_BASE + 10u + (ui.kpage == KP_KSET ? 1u + (ui.kks & 3u) : 0u);
    if (ui.kpage >= KP_TERROR)
        return KP_BASE + 4u + (uint32_t)(ui.kpage - KP_TERROR);
    return ui.kpage == KP_MIX ? 0u : ui.kpage == KP_MACRO ? KP_BASE - 2u : ui.kpage == KP_OUT ? KP_BASE - 1u
           : 1u + ui.klayer * 2u + (ui.kpage == KP_FX ? 1u : 0u);
}
static void kick_part_name(uint32_t p, char *t)
{
    if (p == KP_BASE + 15u) {
        text_cat(t, "CLEAN MORPH");
        return;
    }
    if (p == KP_BASE + 16u) {
        text_cat(t, "STYLE");
        return;
    }
    if (p >= KP_BASE + 11u) {            /* (K1 SET ..: the knob, its function) */
        char k[3] = {'K', (char)('1' + (p - KP_BASE - 11u) % 4u), 0};
        text_cat(t, k);
        text_cat(t, " ");
        text_cat(t, sk_short(selk_fn((p - KP_BASE - 11u) % 4u)));
        return;
    }
    if (p >= KP_BASE) {
        text_cat(t, KP_MORE_NAME[(p - KP_BASE) % 11u]);
        return;
    }
    if (p == 0u || p >= KP_BASE - 2u) {
        text_cat(t, KPAGE_NAME[p == 0u ? KP_MIX : p == KP_BASE - 2u ? KP_MACRO : KP_OUT]);
        return;
    }
    text_cat(t, KICK_LAYER_NAME[(p - 1u) / 2u]);
    if (!((p - 1u) & 1u))
        return;
    text_cat(t, " FX");
}
/* LOAF BREAKER: the KICK menu: its parts in four sections, EDIT the next section, ALGORITHM the next part in it.
 * SOUND: MIX, SUB, PUNCH, GRIT, CLICK, OUT. SYNTH: SYNTH, SYNTH +, MACRO. DIRT: TERROR, TERROR +, TERROR 2, the layers'
 * effects. TONE: EQ BANDS, EQ SHAPE, FX 1, FX 2 */
#define KSECTS 5u
static const char *const KSECT_NAME[KSECTS] = {"SOUND", "SYNTH", "DIRT", "TONE", "KNOBS"};
static const uint8_t KSECT_N[KSECTS] = {6u, 5u, 7u, 4u, 6u};
static const uint8_t KSECT[KSECTS][7] = {{0u, 1u, 3u, 5u, 7u, 10u}, {18u, 19u, 20u, 26u, 9u}, {15u, 16u, 17u, 2u, 4u, 6u, 8u},
                                        {11u, 12u, 13u, 14u}, {21u, 22u, 23u, 24u, 25u, 27u}};   /* (KNOBS: KP_BASE + 10 ..; 26 CLEAN MORPH) */
static void ksect_find(uint32_t p)       /* the part's section and place in it, kept */
{
    uint32_t s, j;
    for (s = 0; s < KSECTS; s++)
        for (j = 0; j < KSECT_N[s]; j++)
            if (KSECT[s][j] == p) {
                ui.ksect = (uint8_t)s;
                ui.kpos[s] = (uint8_t)j;
                return;
            }
}
static void kick_part_set(uint32_t p);
/* LOAF BREAKER TEST: GLO MORE's MODE BEGINNER: ALGORITHM passes over the advanced parts (each as it is, EXPERT shows
 * it again): SOUND MIX, SUB, OUT; SYNTH SYNTH, BASS, MACRO; DIRT TERROR; TONE EQ BANDS, FX 1; KNOBS KNOBS */
#define KP_ADV ((1u << 2) | (1u << 3) | (1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 8) | (1u << 12) | \
                (1u << 14) | (1u << 16) | (1u << 17) | (1u << 19) | (1u << 22) | (1u << 23) | (1u << 24) | (1u << 25) | \
                (1u << 26))
static int kp_hidden(uint32_t p) { return set.novice && p < 32u && (KP_ADV >> p & 1u); }
static void kick_open(void)              /* the KICK menu, at its section's part */
{
    uint32_t s = ui.ksect % KSECTS;
    if (kp_hidden(KSECT[s][ui.kpos[s] % KSECT_N[s]]))
        ui.kpos[s] = 0u;                 /* (BEGINNER: an advanced part left open: the section's first) */
    kick_part_set(KSECT[s][ui.kpos[s] % KSECT_N[s]]);
}
static void kick_sect_next(void)         /* EDIT on the KICK menu */
{
    ui.ksect = (uint8_t)((ui.ksect + 1u) % KSECTS);
    kick_open();
}
static void kick_sect_step(int32_t d)    /* ALGORITHM on it */
{
    uint32_t s = ui.ksect % KSECTS, n = KSECT_N[s], k;
    for (k = 0; k < n; k++) {            /* (BEGINNER: the advanced parts passed over) */
        ui.kpos[s] = (uint8_t)((ui.kpos[s] + (d > 0 ? 1u : n - 1u)) % n);
        if (!kp_hidden(KSECT[s][ui.kpos[s]]))
            break;
    }
    kick_open();
}
static void kick_part_set(uint32_t p)
{
    ksect_find(p);
    ui.page = PG_KICK;
    if (p >= KP_BASE) {                  /* (TEST) the KICK EQ, the kick's FX slots, its TERRORs */
        uint32_t q = (p - KP_BASE) % 17u;
        if (q < 2u) {
            ui.page = PG_KEQ;
            ui.keq_part = (uint8_t)q;
        } else if (q < 4u) {
            if (ui.fx_focus < BRK_FX_SLOTS)
                ui.fx_main = ui.fx_focus;
            ui.fx_focus = (uint8_t)(BRK_FX_SLOTS + q - 2u);
            ui.fx_page = 0u;
            ui.page = PG_FX;
        } else if (q == 15u)
            ui.kpage = KP_CMORPH;
        else if (q == 16u)
            ui.kpage = KP_STYLE;
        else if (q >= 10u) {             /* (KNOBS, K1 SET .. K4 SET) */
            ui.kpage = q == 10u ? KP_KNOBS : KP_KSET;
            ui.kks = (uint8_t)((q - 11u) & 3u);
        } else
            ui.kpage = (uint8_t)(KP_TERROR + q - 4u);
    } else if (p == 0u)
        ui.kpage = KP_MIX;
    else if (p == KP_BASE - 2u)
        ui.kpage = KP_MACRO;
    else if (p == KP_BASE - 1u)
        ui.kpage = KP_OUT;
    else {
        ui.kpage = (p - 1u) & 1u ? KP_FX : KP_LAYER;
        ui.klayer = (uint8_t)((p - 1u) / 2u);
    }
    page_say();
}
static const char *const KN_KICK[KPAGES][4] = {{"SUB", "PUNCH", "GRIT", "CLICK"},
                                               {"LEVEL", "PITCH", "DECAY", "TONE"},
                                               {"EFFECT", "", "", "WET"},
                                               {"SUB", "PUNCH", "GRIT", "INTENSITY"},
                                               {"VOLUME", "TERROR", "TERROR 2", ""},   /* (LOAF BREAKER: OUT's K2, K3) */
                                               {"AMOUNT", "SUB", "FIZZ", "SUSTAIN"},
                                               {"DRIVE", "TYPE", "SCREECH", "TAIL"},
                                               {"AMOUNT", "SUB", "SCREAM", "PUSH"},
                                               {"DROP", "TIME", "SHAPE", "HOLD"},
                                               {"RUMBLE", "LENGTH", "ASYM", "TUNE"},
                                               {"GLIDE", "REV BASS", "START", "CURVE"},
                                               {"", "", "", ""}, {"", "", "", ""},
                                               {"CLEAN SUB", "MORPH", "TO KICK", ""}, {"K1", "K2", "K3", "K4"}};   /* (KNOBS, K SET: kick_knob_name) */
/* LOAF BREAKER: OUT's K2 TERROR, K3 TERROR 2 (kick.h), 0..100 each; SEL + K4 and SEL + K3 on any page (main.c) */
static uint8_t *kick_hc(kick_t *k, int i) { return i == 2 ? &KICK_TERROR2(k) : &KICK_TERROR(k); }
static void kick_hc_text(const kick_t *k, int i, char *t)
{
    uint8_t v = *kick_hc((kick_t *)k, i);
    t[0] = 0;
    if (!v) {
        text_cat(t, "OFF");
        return;
    }
    int_text(v, t);
    text_cat(t, "%");
}
static const char *const KFX_PARAM[KICK_FX][2] = {{"", ""}, {"GAIN", "TONE"}, {"BITS", "RATE"},
                                                  {"CUTOFF", "RESO"}, {"FREQ", ""}};
/* the macros: each moves a few of the sound's settings together */
#define KM_N 3u
static float *kick_macro(int m, uint32_t j)
{
    kick_t *k = &set.kick;
    switch (m) {
    case 0:                              /* SUB: its level, its tail, its weight */
        return j == 0u ? &k->l[KL_SUB].level : j == 1u ? &k->l[KL_SUB].decay : &k->l[KL_SUB].tone;
    case 1:                              /* PUNCH: its level, its sweep, the click */
        return j == 0u ? &k->l[KL_PUNCH].level : j == 1u ? &k->l[KL_PUNCH].tone : &k->l[KL_CLICK].level;
    default:                             /* GRIT: its level, its dirt, its tail */
        return j == 0u ? &k->l[KL_GRIT].level : j == 1u ? &k->l[KL_GRIT].tone : &k->l[KL_GRIT].decay;
    }
}
static float kick_macro_value(int m)
{
    uint32_t j;
    float v = 0.0f;
    for (j = 0; j < KM_N; j++)
        v += *kick_macro(m, j);
    return v * (1.0f / (float)KM_N);
}
/* LOAF BREAKER: a layer's FX page, K3 empty (no effect, or RING's one setting): PRESET, the kick back to a built-in
 * one (909 .. DISTORTO, its volume kept) */
static int kick_preset_knob(int i)
{
    return (ui.kpage == KP_FX && i == 2 && !KFX_PARAM[set.kick.l[ui.klayer % KICK_LAYERS].fx % KICK_FX][1][0]) ||
           (ui.kpage == KP_OUT && i == 3);   /* (LOAF BREAKER: OUT's K4, always) */
}
static const char *kick_knob_name(int i)
{
    if (kick_preset_knob(i))
        return "PRESET";
    if (ui.kpage == KP_KNOBS)
        return sk_short(kn_fn(i));
    if (ui.kpage == KP_KSET)
        return sk_kname(kn_fn(i), i);
    if (ui.kpage == KP_FX && (i == 1 || i == 2))
        return KFX_PARAM[set.kick.l[ui.klayer % KICK_LAYERS].fx % KICK_FX][i - 1];
    return KN_KICK[ui.kpage % KPAGES][i];
}
static void kick_value(int i, char *t)
{
    const kick_t *k = &set.kick;
    const kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    t[0] = 0;
    if (kp_terror()) {                   /* the TERRORs */
        selk_value(kp_fn(), i, t);
        return;
    }
    if (kp_knobs()) {                    /* KNOBS, K SET */
        if (kick_knob_name(i)[0])
            selk_value(kn_fn(i), kn_i(i), t);
        return;
    }
    if (ui.kpage == KP_CMORPH) {         /* CLEAN MORPH */
        if (i < 3)
            selk_value(cm_fn(i), cm_i(i), t);
        return;
    }
    if (ui.kpage == KP_STYLE) {          /* STYLE: each knob's function */
        text_cat(t, sk_short(selk_fn((uint32_t)i)));
        return;
    }
    if (ui.kpage >= KP_SYN1) {           /* SYNTH, SYNTH + */
        ks_value(i, t);
        return;
    }
    switch (ui.kpage) {
    case KP_MIX:
        pct_text(k->l[i].level, t);
        break;
    case KP_LAYER:
        if (i == 1)
            hz_text(kick_pitch_hz(ui.klayer, l->pitch), t);
        else if (i == 2) {
            int_text((int32_t)(kick_decay_ms(ui.klayer, l->decay) + 0.5f), t);
            text_cat(t, " MS");
        } else
            pct_text(i == 0 ? l->level : l->tone, t);
        break;
    case KP_FX:
        if (kick_preset_knob(i))
            kpre_text(t);
        else if (i == 0)
            text_cat(t, KICK_FX_NAME[l->fx % KICK_FX]);
        else if (i == 3)
            pct_text(l->wet, t);
        else if (!kick_knob_name(i)[0])
            ;
        else if (l->fx == KFX_CRUNCH && i == 1) {
            int_text((int32_t)kick_fx_bits(l->fa), t);
            text_cat(t, " BIT");
        } else if ((l->fx == KFX_FILTER || l->fx == KFX_RING) && i == 1)
            hz_text(kick_fx_hz(l->fx, l->fa), t);
        else
            pct_text(i == 1 ? l->fa : l->fb, t);
        break;
    case KP_MACRO:
        pct_text(i == 3 ? k->intensity : kick_macro_value(i), t);
        break;
    default:
        if (kick_preset_knob(i))         /* (LOAF BREAKER: OUT's K4: the built-in kick) */
            kpre_text(t);
        else if (i == 0)
            pct_text(k->volume, t);
        else if (i <= 2)                 /* LOAF BREAKER: TERROR, TERROR 2 */
            kick_hc_text(k, i, t);
        break;
    }
}
/* LOAF BREAKER: a kick layer's effect remembered as it was left (pats.mem), each layer's each effect */
static void kfx_mem_put(uint32_t layer, const kick_layer_t *l)
{
    uint16_t *m;
    if (!l->fx || l->fx >= KICK_FX)
        return;
    m = pats.mem.kfx[layer % KICK_LAYERS][l->fx - 1u];
    m[0] = fxm_q(l->fa);
    m[1] = fxm_q(l->fb);
    m[2] = fxm_q(l->wet);
    pats.mem.kfx_ok |= (uint16_t)(1u << ((layer % KICK_LAYERS) * 4u + l->fx - 1u));
}
static int kfx_mem_get(uint32_t layer, kick_layer_t *l)
{
    const uint16_t *m;
    if (!l->fx || l->fx >= KICK_FX || !(pats.mem.kfx_ok >> ((layer % KICK_LAYERS) * 4u + l->fx - 1u) & 1u))
        return 0;
    m = pats.mem.kfx[layer % KICK_LAYERS][l->fx - 1u];
    l->fa = fxm_f(m[0]);
    l->fb = fxm_f(m[1]);
    l->wet = fxm_f(m[2]);
    return 1;
}
/* SEL clicked twice on EDIT KICK's FX part: the layer's effect back to its start (twice again within 4 s: undone) */
static struct { float fa, fb, wet; uint32_t t; uint8_t layer, fx; } kfx_undo;
static void kfx_reset(void)
{
    uint32_t ly = ui.klayer % KICK_LAYERS, now = plat_ms();
    kick_layer_t *l = &set.kick.l[ly];
    if (kfx_undo.t && now - kfx_undo.t < 4000u && kfx_undo.layer == ly && kfx_undo.fx == l->fx) {
        l->fa = kfx_undo.fa;
        l->fb = kfx_undo.fb;
        l->wet = kfx_undo.wet;
        kfx_undo.t = 0u;
        app_say(KICK_FX_NAME[l->fx % KICK_FX], "RESET UNDONE");
    } else {
        if (!l->fx) {
            app_say("KICK FX", "OFF");
            return;
        }
        kfx_undo.fa = l->fa;
        kfx_undo.fb = l->fb;
        kfx_undo.wet = l->wet;
        kfx_undo.layer = (uint8_t)ly;
        kfx_undo.fx = l->fx;
        kfx_undo.t = now | 1u;
        l->fa = l->fb = 0.5f;
        l->wet = 1.0f;
        pats.mem.kfx_ok &= (uint16_t)~(1u << (ly * 4u + l->fx - 1u));
        app_say(KICK_FX_NAME[l->fx % KICK_FX], "RESET (SEL+REC: UNDO)");
    }
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed_kick();
}
static void kick_knob_turn(int i, int32_t d)
{
    kick_t *k = &set.kick;
    kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    char t[16];
    if (kp_terror()) {                   /* the TERRORs */
        selk_turn(kp_fn(), i, d);
        return;
    }
    if (kp_knobs()) {                    /* KNOBS, K SET */
        if (kick_knob_name(i)[0])
            selk_turn(kn_fn(i), kn_i(i), d);
        return;
    }
    if (ui.kpage == KP_CMORPH) {         /* CLEAN MORPH */
        if (i < 3)
            selk_turn(cm_fn(i), cm_i(i), d);
        return;
    }
    if (ui.kpage == KP_STYLE) {          /* STYLE: K1-K4 each its own knob's function (PRESETS: a style) */
        selk_assign_k((uint32_t)i, d);
        return;
    }
    if (ui.kpage >= KP_SYN1) {           /* SYNTH, SYNTH + */
        char w[24];
        ks_turn(i, d);
        ks_value(i, t);
        w[0] = 0;
        text_cat(w, "KICK ");
        text_cat(w, KN_KICK[ui.kpage % KPAGES][i & 3]);
        app_say(w, t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    }
    if (kick_preset_knob(i)) {           /* the whole kick: every layer, its effects, INTENSITY (the kick selected,
                                          * KICK 1-10: its memory takes it); the volume and the steps kept */
        ui.kpreset = (uint8_t)((ui.kpreset % KPRE_N + (d > 0 ? 1u : KPRE_N - 1u)) % KPRE_N);
        if (ui.kpreset < KICK_PRESETS) {
            uint32_t j;
            kick_preset(k, ui.kpreset);
            for (j = 0; j < KS_N; j++)   /* (LOAF BREAKER: its SYNTH back to as it always was too) */
                set.ksyn[j] = 0;
        } else {                         /* (COPY KICK n: that memory's, if it is another) */
            uint32_t n = ui.kpreset - KICK_PRESETS, j;
            float vol = k->volume;
            kick_value(i, t);
            if (n == set.kslot % KICK_MEMS) {
                app_say(t, "(THIS ONE)");
                ui.redraw |= RG_FOOT;
                return;
            }
            *k = set.kmem[n];
            k->volume = vol;
            for (j = 0; j < KS_N; j++)
                set.ksyn[j] = set.ksmem[n][j];
            kx_get(n);
            app_say(t, "ON THIS KICK");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
            changed_kick();
            return;
        }
        kick_value(i, t);
        app_say("KICK PRESET", t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    }
    switch (ui.kpage) {
    case KP_MIX:
        k->l[i].level = knob_f(k->l[i].level, d, 0.02f, 0.0f, 1.0f);
        break;
    case KP_LAYER: {
        float *p = i == 0 ? &l->level : i == 1 ? &l->pitch : i == 2 ? &l->decay : &l->tone;
        *p = knob_f(*p, d, i == 1 ? 0.01f : 0.02f, 0.0f, 1.0f);
        break;
    }
    case KP_FX:
        if (i == 0) {                    /* (LOAF BREAKER: the effect left remembered, the new one as left) */
            uint32_t f = (uint32_t)clamp((int32_t)l->fx + (d > 0 ? 1 : -1), 0, KICK_FX - 1);
            if (f == l->fx)
                break;
            kfx_mem_put(ui.klayer % KICK_LAYERS, l);
            l->fx = (uint8_t)f;
            if (!kfx_mem_get(ui.klayer % KICK_LAYERS, l)) {
                l->fa = l->fb = 0.5f;    /* (kick_preset's) */
                l->wet = 1.0f;
            }
        } else if (i == 3)
            l->wet = knob_f(l->wet, d, 0.02f, 0.0f, 1.0f);
        else if (kick_knob_name(i)[0]) {
            float *p = i == 1 ? &l->fa : &l->fb;
            *p = knob_f(*p, d, 0.02f, 0.0f, 1.0f);
        }
        break;
    case KP_MACRO:
        if (i == 3)
            k->intensity = knob_f(k->intensity, d, 0.02f, 0.0f, 1.0f);
        else {
            uint32_t j;
            for (j = 0; j < KM_N; j++)
                *kick_macro(i, j) = knob_f(*kick_macro(i, j), d, 0.02f, 0.0f, 1.0f);
        }
        break;
    default:
        if (i == 0)
            k->volume = knob_f(k->volume, d, 0.02f, 0.0f, 1.0f);
        else if (i <= 2) {               /* LOAF BREAKER: TERROR, TERROR 2, 2 % a detent */
            uint8_t *p = kick_hc(k, i);
            *p = (uint8_t)clamp((int32_t)*p + d * 2, 0, 100);
        } else
            return;
        break;
    }
    kick_value(i, t);
    if (ui.kpage == KP_LAYER || (ui.kpage == KP_FX && i)) {   /* "GRIT  DECAY 240 MS" */
        char w[24];
        w[0] = 0;
        text_cat(w, kick_knob_name(i));
        text_cat(w, " ");
        text_cat(w, t);
        app_say(KICK_LAYER_NAME[ui.klayer % KICK_LAYERS], w);
    } else
        app_say(kick_knob_name(i), t);
    ui.redraw |= RG_BLOCKS;
    changed_kick();
}

/* the KICK page's strip: the kick's shape and its layers' levels; then its 16 steps (the one playing outlined),
 * what the page edits and where the kick goes */

/* EDIT's lower strip: the drums side by side (LOAF BREAKER: the clap too), each its own waveform (one hit, worked out
 * again when its sound changes), the one edited outlined, lit on its hits */
#define DW 36u                           /* a drum's waveform: its columns */
static uint8_t drum_px[NDRUMS][DW];
static uint32_t drum_sig[NDRUMS] = {1u, 1u, 1u, 1u, 1u};
static uint32_t sig_of(const void *p, uint32_t n)
{
    uint32_t i, h = 2166136261u;
    for (i = 0; i < n; i++)
        h = (h ^ ((const uint8_t *)p)[i]) * 16777619u;
    return h;
}
static void draw_drums(const brk_status_t *st)
{
    uint32_t i, c;
    for (i = 0; i < NDRUMS; i++) {
        uint32_t sig = i ? sig_of(perc_voice(i - 1u), sizeof(perc_voice_t))
                         : sig_of(&set.kick, sizeof set.kick) ^ sig_of(set.ksyn, sizeof set.ksyn);
        if (sig != drum_sig[i]) {        /* its hit again, over a window that fits its decay, as tall as the box */
            uint32_t pk = 0u;
            drum_sig[i] = sig;
            if (i) {
                float d = perc_decay_ms(i - 1u, perc_voice(i - 1u)->decay) * 1.4f;
                perc_shape(i - 1u, perc_voice(i - 1u), drum_px[i], DW, (uint32_t)brk_clampf(d, 60.0f, 900.0f), 120u);
            } else
                kick_shape(&set.kick, drum_px[0], DW, 600u, 120u);
            for (c = 0; c < DW; c++)
                pk = drum_px[i][c] > pk ? drum_px[i][c] : pk;
            for (c = 0; c < DW && pk; c++)
                drum_px[i][c] = (uint8_t)(drum_px[i][c] * 30u / pk);
        }
    }
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < NDRUMS; i++) {
        int32_t x = 1 + (int32_t)i * 48;
        int sel = i == ui.einst, hit = (st->playing && st->bar64 >= 0 &&
            (drum_now[i][st->bar64 >> 4 & 3] >> (st->bar64 & 15) & 1u)) ||
            (ui.aud_d == i && plat_ms() - ui.aud_t < 150u);   /* (LOAF BREAKER: or just played on its key) */
        char kn[2] = {(char)('1' + i), 0};
        uint16_t ink = hit ? C_BLACK : sel ? C_HI : C_GRAY;
        cv_rect(x, 2, 46, 48, sel ? C_WHITE : C_LINE);
        cv_rect(x + 1, 3, 44, 46, hit ? C_BLOCK_HI : C_BLACK);
        cv_text(x + 5, 4, &FONT_T, DRUM_NAME[i], hit ? C_BLACK : sel ? C_HI : C_GRAY);
        if (!st->playing)                /* (its key, to play it: stopped) */
            cv_text(x + 42 - text_w(&FONT_T, kn), 37, &FONT_T, kn, hit ? C_BLACK : C_DIM);
        cv_rect(x + 5, 33, DW, 1, hit ? C_BLACK : C_LINE);   /* the centre line, the hit mirrored about it */
        for (c = 0; c < DW; c++)
            if (drum_px[i][c])
                cv_rect(x + 5 + (int32_t)c, 33 - drum_px[i][c] / 2, 1, drum_px[i][c] | 1, ink);
    }
    cv_blit(0, 140);
}

/* the kick's page (EDIT): as the snare's and the hat's: its part's four settings as bars (lit), and under them the
 * four layers' levels (the one the part edits lit, amber with an effect on); then the three drums */
static float kick_knob_f(int i)          /* the part's knob i, 0..1 (as its bar) */
{
    const kick_t *k = &set.kick;
    const kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    switch (ui.kpage) {
    case KP_MIX:
        return k->l[i].level;
    case KP_LAYER:
        return i == 0 ? l->level : i == 1 ? l->pitch : i == 2 ? l->decay : l->tone;
    case KP_FX:
        return i == 0 ? (float)l->fx / (float)(KICK_FX - 1) : i == 1 ? l->fa : i == 2 ? l->fb : l->wet;
    case KP_MACRO:
        return i == 3 ? k->intensity : kick_macro_value(i);
    case KP_TERROR:
    case KP_TMORE:
    case KP_TERROR2:
        return sk_norm(kp_fn(), i);
    case KP_SYN1:
    case KP_SYN2:
    case KP_BASS:
        return ks_norm(i);
    case KP_KNOBS:
    case KP_KSET:
        return sk_norm(kn_fn(i), kn_i(i));
    case KP_CMORPH:
        return i < 3 ? sk_norm(cm_fn(i), cm_i(i)) : 0.0f;
    case KP_STYLE:                       /* (STYLE: how far each knob's function is turned, SEL + it live) */
        return sk_norm(selk_fn((uint32_t)i), 0);
    default:
        return i == 0 ? k->volume : i <= 2 ? (float)*kick_hc((kick_t *)k, i) * 0.01f :
               (float)(ui.kpreset % KPRE_N) / (float)(KPRE_N - 1u);   /* (OUT K4: the preset, the copies) */
    }
}
/* LOAF BREAKER: the KICK menu's picture (y 44..143, no waveform of the break above it): the kick as it sounds, its
 * hit over 250 ms drawn large (its wave over its level), the SUB's pitch falling across it (amber: from where its DROP starts to its
 * note), its memory, its note and the layers' levels; under it (y 144..191) the part's four knobs as cards, their
 * values large */
#define KV_W 232u
#define KV_MS 250u                       /* (the picture's time: a quarter of a second) */
static uint8_t kv_px[KV_W];              /* the hit's level, column by column */
static int8_t kv_wave[KV_W];             /* and its wave (the pitch falling: the cycles stretching) */
static uint32_t kv_sig;
static uint32_t kv_t;                    /* (TEST: when the hit was last worked out) */
static uint8_t kv_pend;                  /* (TEST: a change not drawn yet) */
static void kick_picture(int32_t top)
{
    const kick_t *k = &set.kick;
    uint32_t i;
    int32_t x, py = 0;
    float hz = kick_pitch_hz(KL_SUB, k->l[KL_SUB].pitch) * brk_exp2f_c((float)set.ksyn[KS_TUNE] * (1.0f / 12.0f));
    float drop = 2.0f * brk_exp2f_c((float)set.ksyn[KS_DROP] * 0.04f);
    float tms = 120.0f * brk_exp2f_c((float)set.ksyn[KS_TIME] * 0.04f);
    char t[24], u[8];
    cv_oy = -top;
    cv_rect(4, 50, KV_W, 1, C_LINE);     /* (the middle) */
    for (i = 0; i < KV_W; i++) {         /* the hit: its level (dim, mirrored), its wave over it */
        int32_t h = kv_px[i], w = kv_wave[i];
        if (h)
            cv_rect(4 + (int32_t)i, 50 - h, 1, 2 * h + 1, C_BLOCK);
        if (w > 0)
            cv_rect(4 + (int32_t)i, 50 - w, 1, w, C_HI);
        else if (w < 0)
            cv_rect(4 + (int32_t)i, 51, 1, -w, C_HI);
    }
    if (kscope_hit) {                    /* LOAF BREAKER TEST: the kick as it really sounds, its last hit: white over it */
        int32_t pk = 1, v;
        for (i = 0; i < KV_W && i < KSC_W; i++) {
            v = kscope[i];
            pk = (v < 0 ? -v : v) > pk ? (v < 0 ? -v : v) : pk;
        }
        for (i = 0; i < KV_W && i < KSC_W; i++) {
            v = kscope[i] * 34 / pk;
            if (v)
                cv_rect(4 + (int32_t)i, v > 0 ? 50 - v : 50 - v, 1, 1, C_WHITE);
        }
    }
    for (x = 0; x < (int32_t)KV_W; x += 2) {   /* the SUB's pitch: 30 Hz at the bottom .. 1 kHz at the top, as it falls */
        float ms = (float)x * ((float)KV_MS / (float)KV_W), pe = brk_exp2f_c(-6.644f * ms / tms);
        float f = hz * (1.0f + drop * pe);
        int32_t y = 84 - (int32_t)(brk_clampf(brk_log2f(f / 30.0f) / 5.06f, 0.0f, 1.0f) * 68.0f);
        if (x)
            cv_line(2 + x, py, 4 + x, y, C_AMB);
        py = y;
    }
    t[0] = 0;                            /* KICK 4, its note */
    text_cat(t, "KICK ");
    int_text((int32_t)set.kslot + 1, u);
    text_cat(t, u);
    cv_text(6, 2, &FONT_B, t, C_WHITE);
    {                                    /* LOAF BREAKER TEST: what the part is for, in a few words (K SET: the knob and
                                          * its function, "K4 TERROR": the tabs leave it no room) */
        static const char *const D[27] = {"THE 4 LAYERS' LEVELS", "THE LOW BOOM", "THE SUB'S EFFECT", "THE THUMP",
                                          "THE PUNCH'S EFFECT", "THE DIRTY LAYER", "THE GRIT'S EFFECT", "THE ATTACK",
                                          "THE CLICK'S EFFECT", "ALL LAYERS AT ONCE", "VOLUME, BUILT-IN KICKS", "",
                                          "", "", "", "FOLD DISTORTION", "THE DISTORTION'S DETAIL",
                                          "MID SCREAM, SUB KEPT", "PITCH DROP AND BODY", "RUMBLE, TUNE",
                                          "GLIDE, REVERSE BASS", "WHAT SEL + K1-K4 DO", "", "", "", "",
                                          "ROUND SUB, BLEND KICKS"};
        uint32_t p = kick_part();
        t[0] = 0;
        if (p >= KP_BASE + 11u && p <= KP_BASE + 14u)
            kick_part_name(p, t);
        else if (p == KP_BASE + 16u) {   /* (STYLE: the one the knobs have now; PRESETS picks one) */
            text_cat(t, "PRESETS: ");
            text_cat(t, kstyle_name());
        }
        else if (p < 27u)
            text_cat(t, D[p]);
        cv_text(12 + text_w(&FONT_B, "KICK 10"), 4, &FONT_XS, t, kp_knobs() || ui.kpage == KP_STYLE ? C_HI : C_GRAY);
    }
    t[0] = 0;
    int_text((int32_t)(hz + 0.5f), t);
    text_cat(t, " HZ");
    cv_text(234 - text_w(&FONT_XS, t), 4, &FONT_XS, t, C_AMB);
    for (i = 0; i < KICK_LAYERS; i++) {  /* the layers' levels, small, at the foot */
        int32_t lx = 6 + (int32_t)i * 58;
        cv_text(lx, 88, &FONT_XS, KICK_LAYER_NAME[i], C_DIM);
        cv_rect(lx + 30, 92, 24, 3, C_BLOCK);
        cv_rect(lx + 30, 92, (int32_t)(24.0f * k->l[i].level + 0.5f), 3, k->l[i].fx ? C_AMB : C_GRAY);
    }
    cv_oy = 0;
}
static void kick_cards(void)             /* the part's four knobs: its name, its value large, its bar */
{
    uint32_t i;
    char t[16];
    cv_begin(240, 48, C_BLACK);
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        const char *n = kick_knob_name((int)i);
        cv_rect(x, 0, 56, 46, C_BLOCK);
        if (!n[0])
            continue;
        if (ui.kpage == KP_STYLE) {      /* (LOAF BREAKER: STYLE: the function's value too, by its knob's name; too
                                          * long for both, the value alone) */
            selk_value(selk_fn(i), 0, t);
            if (text_w(&FONT_XS, t) <= 34)
                cv_text(x + 4, 3, &FONT_XS, n, C_GRAY);
            cv_text(x + 52 - text_w(&FONT_XS, t), 3, &FONT_XS, t, C_WHITE);
        } else
            cv_text(x + 4, 3, &FONT_XS, n, C_GRAY);
        kick_value((int)i, t);
        cv_text(x + 4, 16, text_w(&FONT_B, t) <= 50 ? &FONT_B : &FONT_XS, t, C_WHITE);
        cv_rect(x + 4, 38, 48, 3, C_BLACK);
        cv_rect(x + 4, 38, (int32_t)(48.0f * brk_clampf(kick_knob_f((int)i), 0.0f, 1.0f) + 0.5f), 3, C_HI);
    }
    cv_blit(0, 144);
}
static void draw_kick_classic(const brk_status_t *st)   /* LOAF BREAKER: the menu as it was (EDIT held: either):
                                          * the part's knobs as bars, the layers' levels, the five drums' shapes */
{
    const kick_t *k = &set.kick;
    uint32_t i;
    char t[24];
    cv_begin(240, 52, C_BLACK);
    t[0] = 0;
    text_cat(t, "KICK  ");
    kick_part_name(kick_part(), t);
    cv_text(6, 0, &FONT_XS, t, C_GRAY);
    for (i = 0; i < 4u; i++) {           /* the part's four settings */
        int32_t x = 6 + (int32_t)i * 58;
        const char *n = kick_knob_name((int)i);
        if (!n[0])
            continue;
        cv_text(x, 16, &FONT_XS, n, C_GRAY);
        cv_rect(x, 29, 52, 3, C_BLOCK);
        cv_rect(x, 29, (int32_t)(52.0f * brk_clampf(kick_knob_f((int)i), 0.0f, 1.0f) + 0.5f), 3, C_HI);
    }
    for (i = 0; i < KICK_LAYERS; i++) {  /* the layers' levels */
        int32_t x = 6 + (int32_t)i * 58;
        int lit = i == ui.klayer && (ui.kpage == KP_LAYER || ui.kpage == KP_FX);   /* (MIX: the row above is them) */
        cv_text(x, 34, &FONT_XS, KICK_LAYER_NAME[i], lit ? C_GRAY : C_DIM);
        cv_rect(x, 47, 52, 3, C_BLOCK);
        cv_rect(x, 47, (int32_t)(52.0f * k->l[i].level + 0.5f), 3, lit ? C_HI : k->l[i].fx ? C_AMB : C_DIM);
    }
    cv_blit(0, 88);
    draw_drums(st);
}
static void draw_kick(const brk_status_t *st)
{
    uint32_t sig = sig_of(&set.kick, sizeof set.kick) ^ sig_of(set.ksyn, sizeof set.ksyn), top;
    if (ui.kclassic) {
        draw_kick_classic(st);
        return;
    }
    if (sig != kv_sig && plat_ms() - kv_t < 100u)
        kv_pend = 1u;                    /* (TEST: a knob turning: the hit again at most every 100 ms, the last one
                                          * drawn then: main.c) */
    else if (sig != kv_sig) {            /* (its hit again, only when it changed) */
        uint32_t pk = 0u;
        kv_sig = sig;
        kv_t = plat_ms();
        kv_pend = 0u;
        kick_shape(&set.kick, kv_px, KV_W, KV_MS, 34u);
        kick_wave(&set.kick, kv_wave, KV_W, KV_MS, 34u);
        for (top = 0; top < KV_W; top++)
            pk = kv_px[top] > pk ? kv_px[top] : pk;
        for (top = 0; pk && top < KV_W; top++) {   /* (as tall as it goes) */
            kv_px[top] = (uint8_t)((uint32_t)kv_px[top] * 34u / pk);
            kv_wave[top] = (int8_t)((int32_t)kv_wave[top] * 34 / (int32_t)pk);
        }
    }
    for (top = 0; top < 100u; top += 60u) {   /* (y 44..143, strips of 60 rows) */
        uint32_t h = 100u - top < 60u ? 100u - top : 60u;
        cv_begin(240, h, C_BLACK);
        kick_picture((int32_t)top);
        cv_blit(0, 44u + top);
    }
    kick_cards();
}

/* LOAF BREAKER: HOME's third part, the KICK EQ: the FX chain's EQ on the kick alone (its curve drawn as the FX page
 * draws it). K1-K4: LOW, MID, HIGH, FREQ; ALGORITHM, their shape: LO FREQ, MID Q, HI FREQ, LEVEL */
static const char *const KEQ_NAME[2][4] = {{"LOW", "MID", "HIGH", "FREQ"}, {"LO FREQ", "MID Q", "HI FREQ", "LEVEL"}};
static void keq_value(int i, char *t)
{
    uint32_t j = (ui.keq_part & 1u) * 4u + (uint32_t)(i & 3);
    float v = set.keq[j];
    t[0] = 0;
    if (j < 3u || j == 7u) {             /* -12 .. +12 dB, flat in the middle */
        float db = (v - 0.5f) * 24.0f;
        if (db > -0.25f && db < 0.25f)
            text_cat(t, "0 DB");
        else
            signed_tenths(db, "DB", t);
    } else if (j == 5u)
        hundredths_text(0.3f * brk_exp2f(v * 3.737f), t);
    else
        hz_text(j == 3u ? 200.0f * brk_exp2f(v * 4.64f) : j == 4u ? 40.0f * brk_exp2f(v * 3.3219f)
                                                        : 2000.0f * brk_exp2f(v * 3.0f), t);
}
static void keq_turn(int i, int32_t d)
{
    uint32_t j = (ui.keq_part & 1u) * 4u + (uint32_t)(i & 3);
    char t[16];
    set.keq[j] = knob_f(set.keq[j], d, 0.02f, 0.0f, 1.0f);
    changed_kick();
    keq_value(i, t);
    app_say(KEQ_NAME[ui.keq_part & 1u][i & 3], t);
    ui.redraw |= RG_BLOCKS;
}
static void draw_keq(void)
{
    const float *k = set.keq;
    float p[4], xx[4];
    int32_t x, y, yb, py = 0;
    uint32_t j;
    char t[16];
    for (j = 0; j < 4u; j++) {
        p[j] = k[j];
        xx[j] = k[4u + j];
    }
    cv_begin(240, 52, C_BLACK);
    cv_text(6, 0, &FONT_XS, ui.keq_part ? "KICK EQ   SHAPE (ALGORITHM)" : "KICK EQ   BANDS (ALGORITHM)", C_GRAY);
    for (x = 0; x < 240; x += 4)         /* 0 dB */
        cv_rect(x, 32, 2, 1, C_LINE);
    for (x = 0; x < 240; x++) {          /* the curve, as the FX page's EQ draws it */
        float in, base, v = x_plot(FXT_EQ, x, p, xx, 0u, &in, &base);
        y = 32 - (int32_t)(brk_clampf(v, -1.0f, 1.0f) * 19.0f);
        yb = 32 - (int32_t)(base * 19.0f);
        cv_rect(x, y < yb ? y : yb, 1, y < yb ? yb - y : y - yb, MB_DIM[1]);
        if (x)
            cv_line(x - 1, py, x, y, MB_COL[1]);
        py = y;
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    for (j = 0; j < 8u; j++) {           /* the eight settings, the part K1-K4 turn lit */
        int lit = (j / 4u) == (ui.keq_part & 1u);
        int32_t px = 6 + (int32_t)(j % 4u) * 58, pyy = 2 + (int32_t)(j / 4u) * 25;
        uint8_t save = ui.keq_part;
        ui.keq_part = (uint8_t)(j / 4u);
        keq_value((int)(j % 4u), t);
        ui.keq_part = save;
        cv_text(px, pyy, &FONT_XS, KEQ_NAME[j / 4u][j % 4u], lit ? C_GRAY : C_DIM);
        cv_text(px, pyy + 10, &FONT_T, t, lit ? C_WHITE : C_DIM);
    }
    cv_blit(0, 140);
}
