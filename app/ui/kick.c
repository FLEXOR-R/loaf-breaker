/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the KICK page (EDIT): the kick's layers, effects, macros and output. (Part of app.c's unity: included
 * there in order.) */

/* ---- the KICK page (PRD K-*): EDIT opens it, EDIT again the next sub-page. The white keys are the kick's 16
 * steps, ALGORITHM the layer, PRESETS the built-in kicks, SEL + EDIT through the FX chain or around it */
enum { KP_MIX, KP_LAYER, KP_FX, KP_MACRO, KP_OUT, KP_TERROR, KP_TMORE, KP_TERROR2, KPAGES };
/* (LOAF BREAKER TEST: TERROR, TERROR +, TERROR 2 the kick's own settings of them, ENV's KNOB functions 3, 4, 1) */
static void selk_turn(uint32_t f, int i, int32_t d);
static void selk_value(uint32_t f, int i, char *t);
static float sk_norm(uint32_t f, int i);
static uint32_t kp_fn(void) { return ui.kpage == KP_TERROR ? 3u : ui.kpage == KP_TMORE ? 4u : 1u; }

/* LOAF BREAKER: the kick memories, KICK 1..10 (KICK's PRESETS, SEQ's SELECT): the one playing kept as it is, the next
 * as it was left (the volume kept) */
static void kick_mem_turn(int32_t d)
{
    float vol = set.kick.volume;
    char t[4];
    set.kmem[set.kslot % KICK_MEMS] = set.kick;
    set.kslot = (uint8_t)((set.kslot % KICK_MEMS + (d > 0 ? 1u : KICK_MEMS - 1u)) % KICK_MEMS);
    set.kick = set.kmem[set.kslot];
    set.kick.volume = vol;
    int_text((int32_t)set.kslot + 1, t);
    app_say("KICK", t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed();
}
static const char *const KPAGE_NAME[KPAGES] = {"MIX", "LAYER", "LAYER FX", "MACRO", "OUT", "TERROR", "TERROR +",
                                                "TERROR 2"};
/* the kick's parts in a row, as ALGORITHM turns through them: MIX, SUB, SUB FX, PUNCH, PUNCH FX, GRIT, GRIT FX,
 * CLICK, CLICK FX, MACRO, OUT (each a sub-page, and for a layer its layer); LOAF BREAKER TEST: then EQ BANDS, EQ
 * SHAPE (the KICK EQ page), FX 1, FX 2 (the FX page on the kick's slots), TERROR, TERROR +, TERROR 2 */
#define KP_BASE (3u + 2u * KICK_LAYERS)
#define KPARTS (KP_BASE + 7u)
static const char *const KP_MORE_NAME[7] = {"EQ BANDS", "EQ SHAPE", "FX 1", "FX 2", "TERROR", "TERROR +", "TERROR 2"};
static uint32_t kick_part(void)
{
    if (ui.page == PG_KEQ)
        return KP_BASE + (ui.keq_part & 1u);
    if (ui.page == PG_FX)
        return KP_BASE + 2u + (uint32_t)(ui.fx_focus - BRK_FX_SLOTS) % 2u;
    if (ui.kpage >= KP_TERROR)
        return KP_BASE + 4u + (uint32_t)(ui.kpage - KP_TERROR);
    return ui.kpage == KP_MIX ? 0u : ui.kpage == KP_MACRO ? KP_BASE - 2u : ui.kpage == KP_OUT ? KP_BASE - 1u
           : 1u + ui.klayer * 2u + (ui.kpage == KP_FX ? 1u : 0u);
}
static void kick_part_name(uint32_t p, char *t)
{
    if (p >= KP_BASE) {
        text_cat(t, KP_MORE_NAME[(p - KP_BASE) % 7u]);
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
static void kick_part_set(uint32_t p)
{
    ui.page = PG_KICK;
    if (p >= KP_BASE) {                  /* (TEST) the KICK EQ, the kick's FX slots, its TERRORs */
        uint32_t q = (p - KP_BASE) % 7u;
        if (q < 2u) {
            ui.page = PG_KEQ;
            ui.keq_part = (uint8_t)q;
        } else if (q < 4u) {
            if (ui.fx_focus < BRK_FX_SLOTS)
                ui.fx_main = ui.fx_focus;
            ui.fx_focus = (uint8_t)(BRK_FX_SLOTS + q - 2u);
            ui.fx_page = 0u;
            ui.page = PG_FX;
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
                                               {"AMOUNT", "SUB", "SCREAM", "PUSH"}};
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
    return ui.kpage == KP_FX && i == 2 && !KFX_PARAM[set.kick.l[ui.klayer % KICK_LAYERS].fx % KICK_FX][1][0];
}
static const char *kick_knob_name(int i)
{
    if (kick_preset_knob(i))
        return "PRESET";
    if (ui.kpage == KP_FX && (i == 1 || i == 2))
        return KFX_PARAM[set.kick.l[ui.klayer % KICK_LAYERS].fx % KICK_FX][i - 1];
    return KN_KICK[ui.kpage % KPAGES][i];
}
static void kick_value(int i, char *t)
{
    const kick_t *k = &set.kick;
    const kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    t[0] = 0;
    if (ui.kpage >= KP_TERROR) {         /* (TEST) the TERRORs */
        selk_value(kp_fn(), i, t);
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
            text_cat(t, KICK_PRESET_NAME[ui.kpreset % KICK_PRESETS]);
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
        if (i == 0)
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
        app_say(KICK_FX_NAME[l->fx % KICK_FX], "RESET (SEL X2: UNDO)");
    }
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed();
}
static void kick_knob_turn(int i, int32_t d)
{
    kick_t *k = &set.kick;
    kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    char t[16];
    if (ui.kpage >= KP_TERROR) {         /* (TEST) the TERRORs */
        selk_turn(kp_fn(), i, d);
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
        if (kick_preset_knob(i)) {       /* (the whole kick: every layer, its effects, INTENSITY) */
            ui.kpreset = (uint8_t)((ui.kpreset + (d > 0 ? 1u : KICK_PRESETS - 1u)) % KICK_PRESETS);
            kick_preset(k, ui.kpreset);
            kick_value(i, t);
            app_say("KICK PRESET", t);
            ui.redraw |= RG_BLOCKS | RG_FOOT;
            changed();
            return;
        }
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
    changed();
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
        uint32_t sig = i ? sig_of(perc_voice(i - 1u), sizeof(perc_voice_t)) : sig_of(&set.kick, sizeof set.kick);
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
        int sel = i == ui.einst, hit = st->playing && st->bar64 >= 0 &&
            (drum_now[i][st->bar64 >> 4 & 3] >> (st->bar64 & 15) & 1u);
        uint16_t ink = hit ? C_BLACK : sel ? C_HI : C_GRAY;
        cv_rect(x, 2, 46, 48, sel ? C_WHITE : C_LINE);
        cv_rect(x + 1, 3, 44, 46, hit ? C_BLOCK_HI : C_BLACK);
        cv_text(x + 5, 4, &FONT_T, DRUM_NAME[i], hit ? C_BLACK : sel ? C_HI : C_GRAY);
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
    default:
        return i == 0 ? k->volume : i <= 2 ? (float)*kick_hc((kick_t *)k, i) * 0.01f : 0.0f;
    }
}
static void draw_kick(const brk_status_t *st)
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
    changed();
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
