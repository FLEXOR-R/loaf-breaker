/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's kick (kick.h). */
#include "kick.h"
#include "brk.h"
#include "brk_math.h"

const char *const KICK_LAYER_NAME[KICK_LAYERS] = {"SUB", "PUNCH", "GRIT", "CLICK"};
const char *const KICK_FX_NAME[KICK_FX] = {"OFF", "DRIVE", "CRUNCH", "FILTER", "RING MOD"};
const char *const KICK_PRESET_NAME[KICK_PRESETS] = {"909", "TECHNO", "EARLY HC", "ROTTERDAM", "INDUSTRIAL", "DISTORTO"};

static inline float soft(float x) { return x / (1.0f + brk_absf(x)); }

/* one kick sounding, its layers' state (the screen's shape renders its own) */
typedef struct {
    float ph[KICK_LAYERS], ph2, env[KICK_LAYERS], pe, pe2;
    float lp;                            /* CLICK: its noise's low-pass (what it keeps is above it) */
    float tlo, tlp, tenv;                /* LOAF BREAKER: TERROR's low split, its fizz's low-pass, the kick's level */
    float t2lo, t2lp, t2env;             /* TERROR 2's low split, its mids' low-pass, the kick's level */
    float sc1, sc2;                      /* (TEST) TERROR's SCREECH, its band's state */
    uint32_t hold;                       /* (SYNTH) the SUB's HOLD left, samples (~0u: from the hit, to be set) */
    float rlp, rout, rlvl, dc;           /* the RUMBLE: its input low-passed, its output low-passed, its level; ASYM's
                                          * offset followed (taken off) */
    float pm;                            /* (crossbreed) this hit's pitch, a factor (kick_pitches): 1 as set */
    float s1[KICK_LAYERS], s2[KICK_LAYERS], fph[KICK_LAYERS], fhold[KICK_LAYERS];   /* the layers' effects */
    uint32_t rng;
    float last, declick;
} kv_t;

/* the block's settings, mapped */
typedef struct {
    float f[KICK_LAYERS];                /* each layer's frequency step (cycles a sample) */
    float dec[KICK_LAYERS];              /* each envelope's factor a sample */
    float lvl[KICK_LAYERS];
    float sub_sat, punch_sweep, grit_index, grit_noise, click_noise;
    float click_k;                       /* the click's high-pass coefficient */
    float pe_dec, pe2_dec;               /* the pitch sweeps' factors */
    /* the layers' effects */
    uint8_t fx[KICK_LAYERS];
    float fa[KICK_LAYERS], fb[KICK_LAYERS], wet[KICK_LAYERS];
    float ga[KICK_LAYERS], gb[KICK_LAYERS], gc[KICK_LAYERS];   /* the effect's own coefficients */
    float g, hard, make, vol;            /* the sum: drive, how hard the clip, make-up, volume */
    float tmix, tg;                      /* LOAF BREAKER: TERROR, its mix and its drive (0 mix: off) */
    float t2mix, t2g;                    /* TERROR 2, its mix and its drive */
    float tsub, tfz, tsus;               /* (ENV's KNOB) TERROR's split, its fizz's low-pass, its follow */
    float t2sub, t2scr, t2push;          /* TERROR 2's split, its mids' low-pass, their push */
    float trel;                          /* (TEST) TERROR's TAIL: its level's fall a sample */
    uint8_t ttype;                       /* its TYPE */
    float sa1, sa2, sa3, sgain;          /* its SCREECH: the band's coefficients and level (0: off) */
    float sdrop, asym, rum, rfb;         /* (SYNTH) the SUB's drop, the uneven drive, the rumble's level and feedback */
    uint32_t hold_n, rlen;               /* the SUB's hold (samples), the rumble's line (samples) */
} kc_t;

static volatile kick_t p_k;
static volatile int8_t p_syn[KS_N];      /* (SYNTH) the main loop's */
static int16_t *rum_buf;                 /* the rumble's line (the app's), its length, where it is */
static uint32_t rum_cap, rum_pos;
void kick_set_synth(const int8_t *s)
{
    uint32_t i;
    for (i = 0; i < KS_N; i++)
        p_syn[i] = s[i];
}
void kick_rumble_buf(int16_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
        buf[i] = 0;
    rum_buf = buf;
    rum_cap = len;
    rum_pos = 0u;
}
static volatile uint32_t p_hits;         /* a count: kick_hit */
static uint32_t hits_seen;
static kv_t kv;
static const float *ks_sp;               /* LOAF BREAKER: the vinyl's speed (kick_speed), 0 none */
static float ks_ph, ks_a, ks_b, ks_last; /* its phase between two of the kick's samples, the two; the last out */
static uint8_t ks_on;

/* the factor a sample that falls 40 dB (to where a kick is heard as gone) in ms milliseconds */
static float decay_factor(float ms) { return brk_exp2f_c(-6.644f / (ms * 0.001f * (float)BRK_SR)); }

/* the knobs' 0..1 as each layer's frequency and its decay (to -40 dB) */
static const float P_LO[KICK_LAYERS] = {30.0f, 50.0f, 40.0f, 1000.0f};
static const float P_OCT[KICK_LAYERS] = {1.585f, 2.0f, 3.32f, 3.585f};
static const float D_LO[KICK_LAYERS] = {60.0f, 20.0f, 20.0f, 1.0f};
static const float D_HI[KICK_LAYERS] = {1500.0f, 400.0f, 800.0f, 40.0f};
BRK_COLD float kick_pitch_hz(uint32_t layer, float p)
{
    layer %= KICK_LAYERS;
    return P_LO[layer] * brk_exp2f_c(brk_clampf(p, 0.0f, 1.0f) * P_OCT[layer]);
}
BRK_COLD float kick_decay_ms(uint32_t layer, float d)
{
    layer %= KICK_LAYERS;
    d = brk_clampf(d, 0.0f, 1.0f);
    return D_LO[layer] + (D_HI[layer] - D_LO[layer]) * d * d;
}

BRK_COLD float kick_fx_hz(uint32_t fx, float a)
{
    a = brk_clampf(a, 0.0f, 1.0f);
    return fx == KFX_FILTER ? 60.0f * brk_exp2f_c(a * 7.6f) : 30.0f * brk_exp2f_c(a * 6.0f);   /* 60..11.6k; 30..1.9k */
}
uint32_t kick_fx_bits(float a) { return 15u - (uint32_t)(brk_clampf(a, 0.0f, 1.0f) * 12.0f + 0.5f); }   /* 15..3 */

BRK_COLD static void kc_make(const kick_t *k, kc_t *c)
{
    uint32_t i;
    for (i = 0; i < KICK_LAYERS; i++) {
        const kick_layer_t *l = &k->l[i];
        float fa = brk_clampf(l->fa, 0.0f, 1.0f), fb = brk_clampf(l->fb, 0.0f, 1.0f);
        c->f[i] = kick_pitch_hz(i, l->pitch) / (float)BRK_SR;
        c->dec[i] = decay_factor(kick_decay_ms(i, l->decay));
        c->lvl[i] = brk_clampf(l->level, 0.0f, 1.0f);
        c->fx[i] = l->fx < KICK_FX ? l->fx : KFX_OFF;
        c->fa[i] = fa;
        c->fb[i] = fb;
        c->wet[i] = brk_clampf(l->wet, 0.0f, 1.0f);
        switch (c->fx[i]) {
        case KFX_DRIVE:                  /* ga: gain, gb: its tone's coefficient */
            c->ga[i] = 1.0f + 24.0f * fa * fa;
            c->gb[i] = 0.05f + 0.95f * fb * fb;
            break;
        case KFX_CRUNCH:                 /* ga: levels, gb: the rate's step */
            c->ga[i] = brk_exp2f_c((float)kick_fx_bits(fa) - 1.0f);
            c->gb[i] = brk_exp2f_c(-fb * 4.5f);
            break;
        case KFX_FILTER: {               /* a 2-pole low-pass: ga g, gb k (resonance), gc a1 */
            float g = brk_tan_pi_c(kick_fx_hz(KFX_FILTER, fa) / (float)BRK_SR);
            c->gb[i] = 2.0f - 1.9f * fb;
            c->gc[i] = 1.0f / (1.0f + g * (g + c->gb[i]));
            c->ga[i] = g;
            break;
        }
        case KFX_RING:
            c->ga[i] = kick_fx_hz(KFX_RING, fa) / (float)BRK_SR;
            break;
        default:
            break;
        }
    }
    c->sub_sat = 1.0f + 4.0f * brk_clampf(k->l[KL_SUB].tone, 0.0f, 1.0f);
    c->punch_sweep = 3.0f + 14.0f * brk_clampf(k->l[KL_PUNCH].tone, 0.0f, 1.0f);
    c->grit_index = 0.5f + 7.0f * brk_clampf(k->l[KL_GRIT].tone, 0.0f, 1.0f);
    c->grit_noise = 0.5f * brk_clampf(k->l[KL_GRIT].tone, 0.0f, 1.0f);
    c->click_noise = brk_clampf(k->l[KL_CLICK].tone, 0.0f, 1.0f);
    c->click_k = brk_clampf(brk_tan_pi_c(c->f[KL_CLICK]) * 2.0f, 0.02f, 0.98f);
    {                                    /* (SYNTH: 0, as it always was) */
        float dr = (float)p_syn[KS_DROP], tm = (float)p_syn[KS_TIME], sh = (float)p_syn[KS_SHAPE];
        float hd = (float)p_syn[KS_HOLD], rm = (float)p_syn[KS_RUMBLE], rt = (float)p_syn[KS_RTIME];
        float tune = brk_exp2f_c(brk_clampf((float)p_syn[KS_TUNE], -12.0f, 12.0f) * (1.0f / 12.0f));
        uint32_t j;
        c->sdrop = 2.0f * brk_exp2f_c(brk_clampf(dr, -50.0f, 50.0f) * 0.04f);
        c->pe_dec = decay_factor(120.0f * brk_exp2f_c(brk_clampf(tm, -50.0f, 50.0f) * 0.04f));
        c->sub_sat *= brk_exp2f_c(brk_clampf(sh, -50.0f, 50.0f) * 0.06f);
        c->hold_n = hd > 0.0f ? (uint32_t)(brk_clampf(hd, 0.0f, 50.0f) * 5.0f * (float)(BRK_SR / 1000u)) : 0u;
        if (hd < 0.0f)                   /* (below 0: the SUB decays faster) */
            c->dec[KL_SUB] = decay_factor(kick_decay_ms(KL_SUB, k->l[KL_SUB].decay) *
                                          brk_exp2f_c(brk_clampf(hd, -50.0f, 0.0f) * 0.02f));
        c->rum = rm > 0.0f ? brk_clampf(rm, 0.0f, 50.0f) * 0.02f : 0.0f;
        c->rlen = (uint32_t)(2048.0f * brk_exp2f_c(brk_clampf(rt, -50.0f, 50.0f) * 0.02f));
        if (c->rlen > rum_cap)
            c->rlen = rum_cap;
        c->rfb = 0.75f + brk_clampf(rt, -50.0f, 50.0f) * 0.003f;
        c->asym = p_syn[KS_ASYM] > 0 ? (float)p_syn[KS_ASYM] * 0.005f : 0.0f;
        for (j = 0; j < KICK_LAYERS; j++)
            c->f[j] *= tune;
    }
    if (!c->pe_dec)
        c->pe_dec = decay_factor(120.0f);     /* SUB's drop and GRIT's */
    c->pe2_dec = decay_factor(40.0f);     /* PUNCH's fast sweep */
    {
        float in = brk_clampf(k->intensity, 0.0f, 1.0f), sg;
        c->g = 0.3f + 30.0f * in * in;   /* (0: near clean, the clip only rounding the peaks) */
        c->hard = in > 0.5f ? (in - 0.5f) * 1.6f : 0.0f;
        sg = c->g / (1.0f + c->g);       /* a full-scale sum through the clip: kept there */
        c->make = 0.95f / (sg * (1.0f - c->hard) + c->hard);
    }
    {                                    /* LOAF BREAKER: TERROR, in fully by a third of the way, the drive on up */
        float t = (float)(KICK_TERROR(k) > 100u ? 100u : KICK_TERROR(k)) * 0.01f;
        c->tmix = t * 3.0f > 1.0f ? 1.0f : t * 3.0f;
        c->tg = (2.0f + 60.0f * t * t) * brk_exp2f_c((float)(int8_t)KICK_T_DRIVE(k) * 0.04f);   /* (TEST: DRIVE) */
    }
    {                                    /* (TEST) TERROR's TYPE, TAIL, SCREECH */
        float tl = brk_clampf((float)(int8_t)KICK_T_TAIL(k), -50.0f, 50.0f), sw = (float)KICK_T_SCREECH(k);
        c->ttype = (uint8_t)(KICK_T_TYPE(k) <= 2u ? KICK_T_TYPE(k) : 0u);
        c->trel = 0.0012f * brk_exp2f_c(-tl * 0.07f);   /* (+50: ~11x slower, the fold hangs on; -50: faster) */
        c->sgain = 0.0f;
        if (sw > 0.0f) {                 /* a band at ~500 Hz .. 5 kHz, Q ~6 (Simper's state-variable) */
            float g = brk_tan_pi_c(500.0f * brk_exp2f_c(brk_clampf(sw, 1.0f, 100.0f) * 0.0332f) / (float)BRK_SR), kq = 1.0f / 6.0f;
            c->sa1 = 1.0f / (1.0f + g * (g + kq));
            c->sa2 = g * c->sa1;
            c->sa3 = g * c->sa2;
            c->sgain = 0.5f + 1.5f * sw * 0.01f;
        }
    }
    {                                    /* LOAF BREAKER: TERROR 2, in fully by a quarter of the way, the drive on up */
        float t = (float)(KICK_TERROR2(k) > 100u ? 100u : KICK_TERROR2(k)) * 0.01f;
        c->t2mix = t * 4.0f > 1.0f ? 1.0f : t * 4.0f;
        c->t2g = 3.0f + 80.0f * t * t;
    }
    {                                    /* (their settings: 0 as they always were) */
        float a = (float)(int8_t)KICK_T_SUB(k), b = (float)(int8_t)KICK_T_FIZZ(k), e = (float)(int8_t)KICK_T_SUS(k);
        c->tsub = 0.0155f * brk_exp2f_c(brk_clampf(a, -50.0f, 50.0f) * 0.024f);
        c->tfz = brk_clampf(0.45f * brk_exp2f_c(brk_clampf(b, -50.0f, 50.0f) * 0.03f), 0.03f, 1.0f);
        c->tsus = 2.5f * brk_exp2f_c(brk_clampf(e, -50.0f, 50.0f) * 0.04f);
        a = (float)(int8_t)KICK_T2_SUB(k);
        b = (float)(int8_t)KICK_T2_SCREAM(k);
        e = (float)(int8_t)KICK_T2_PUSH(k);
        c->t2sub = 0.0155f * brk_exp2f_c(brk_clampf(a, -50.0f, 50.0f) * 0.024f);
        c->t2scr = brk_clampf(0.085f * brk_exp2f_c(brk_clampf(b, -50.0f, 50.0f) * 0.04f), 0.01f, 0.6f);
        c->t2push = 1.2f * brk_exp2f_c(brk_clampf(e, -50.0f, 50.0f) * 0.03f);
    }
    c->vol = brk_clampf(k->volume, 0.0f, 1.0f);
}

static void kv_hit(kv_t *v)
{
    uint32_t i;
    v->declick = v->last;                /* the kick still sounding fades out under the new one */
    for (i = 0; i < KICK_LAYERS; i++) {
        v->ph[i] = 0.0f;
        v->env[i] = 1.0f;
        v->s1[i] = v->s2[i] = v->fph[i] = v->fhold[i] = 0.0f;
    }
    v->ph2 = 0.0f;
    v->pe = v->pe2 = 1.0f;
    v->lp = 0.0f;
    v->hold = 0xFFFFFFFFu;               /* (SYNTH: its HOLD, set at the first sample) */
}

static inline float noise(kv_t *v)
{
    v->rng = v->rng * 1664525u + 1013904223u;
    return (float)(int32_t)v->rng * (1.0f / 2147483648.0f);
}

/* a layer's own effect (on x, -1..1) */
static inline float layer_fx(kv_t *v, const kc_t *c, uint32_t i, float x)
{
    float y = x;
    switch (c->fx[i]) {
    case KFX_DRIVE:
        v->s1[i] += (soft(x * c->ga[i]) - v->s1[i]) * c->gb[i];
        y = v->s1[i];
        break;
    case KFX_CRUNCH:
        v->fph[i] += c->gb[i];
        if (v->fph[i] >= 1.0f) {
            v->fph[i] -= 1.0f;
            v->fhold[i] = (float)(int32_t)(x * c->ga[i]) / c->ga[i];
        }
        y = v->fhold[i];
        break;
    case KFX_FILTER: {                   /* (Simper's state-variable low-pass) */
        float g = c->ga[i], a1 = c->gc[i], a2 = g * a1, a3 = g * a2;
        float v3 = x - v->s2[i], v1 = a1 * v->s1[i] + a2 * v3, v2 = v->s2[i] + a2 * v->s1[i] + a3 * v3;
        v->s1[i] = 2.0f * v1 - v->s1[i];
        v->s2[i] = 2.0f * v2 - v->s2[i];
        y = v2;
        break;
    }
    case KFX_RING:
        y = x * brk_sin_turns(v->fph[i]);
        v->fph[i] += c->ga[i];
        if (v->fph[i] >= 1.0f)
            v->fph[i] -= 1.0f;
        break;
    default:
        return x;
    }
    return x + (y - x) * c->wet[i];
}

static inline float kv_tick(kv_t *v, const kc_t *c)
{
    float x[KICK_LAYERS], y = 0.0f, d, out;
    uint32_t i;
    /* SUB: a sine dropping an octave and a half to its pitch, saturated by its tone */
    x[KL_SUB] = soft(brk_sin_turns(v->ph[KL_SUB]) * c->sub_sat) * (1.0f + c->sub_sat) / c->sub_sat;
    v->ph[KL_SUB] += c->f[KL_SUB] * v->pm * (1.0f + c->sdrop * v->pe);   /* (SYNTH: its DROP) */
    if (v->hold == 0xFFFFFFFFu)
        v->hold = c->hold_n;
    /* PUNCH: a sine swept down fast from far above */
    x[KL_PUNCH] = brk_sin_turns(v->ph[KL_PUNCH]);
    v->ph[KL_PUNCH] += c->f[KL_PUNCH] * v->pm * (1.0f + c->punch_sweep * v->pe2);
    /* GRIT: FM (the modulator at 1.41x), driven, noise by its tone */
    x[KL_GRIT] = soft(4.0f * (brk_sin_turns(v->ph[KL_GRIT] + c->grit_index * 0.159f * brk_sin_turns(v->ph2)) +
                              c->grit_noise * noise(v)));
    v->ph[KL_GRIT] += c->f[KL_GRIT] * v->pm * (1.0f + v->pe);
    v->ph2 += c->f[KL_GRIT] * v->pm * 1.41f * (1.0f + v->pe);
    /* CLICK: noise high-passed at its pitch, and a click ringing there; tone between them */
    {
        float n = noise(v), ring = brk_sin_turns(v->ph[KL_CLICK]);
        v->lp += (n - v->lp) * c->click_k;
        x[KL_CLICK] = (n - v->lp) * c->click_noise + ring * (1.0f - c->click_noise);
        v->ph[KL_CLICK] += c->f[KL_CLICK];
    }
    for (i = 0; i < KICK_LAYERS; i++) {
        if (v->ph[i] >= 1.0f)
            v->ph[i] -= (float)(uint32_t)v->ph[i];
        y += layer_fx(v, c, i, x[i] * v->env[i]) * c->lvl[i];
        if (i == KL_SUB && v->hold)      /* (SYNTH: the SUB held at full) */
            v->hold--;
        else
            v->env[i] *= c->dec[i];
    }
    if (v->ph2 >= 1.0f)
        v->ph2 -= (float)(uint32_t)v->ph2;
    v->pe *= c->pe_dec;
    v->pe2 *= c->pe2_dec;
    /* the sum: a drive into a clip, soft and then (INTENSITY over half) hard: the distorted kick */
    d = y * c->g;
    if (c->asym > 0.0f)                  /* (SYNTH: ASYM, uneven: one side pushed harder) */
        d += c->asym * d * brk_absf(d);
    out = (soft(d) * (1.0f - c->hard) + brk_clampf(d, -1.0f, 1.0f) * c->hard) * c->make;
    if (c->asym > 0.0f) {                /* (its offset taken off) */
        v->dc += (out - v->dc) * 0.0005f;
        out -= v->dc;
    }
    if (c->tmix > 0.0f) {                /* LOAF BREAKER: TERROR: the sub (below ~110 Hz) kept, the rest driven into a
                                          * triangle fold (it folds back instead of flattening: the scream), then
                                          * clipped and its fizz rounded */
        float h, t, z, a = brk_absf(out);
        v->tenv += (a - v->tenv) * (a > v->tenv ? 0.05f : c->trel);   /* (the kick's level: fast up, ~20 ms down;
                                          * TAIL: slower or faster) */
        v->tlo += (out - v->tlo) * c->tsub;
        h = (out - v->tlo) * c->tg;
        t = h * 0.25f + 0.25f;
        t -= (float)(int32_t)t;
        if (t < 0.0f)
            t += 1.0f;
        z = 1.0f - 4.0f * brk_absf(t - 0.5f);   /* (the fold: h within +-1 untouched, beyond it back and forth) */
        if (c->ttype == 1u)              /* (TEST) CLIP: squared off instead */
            z = brk_clampf(h, -1.0f, 1.0f);
        else if (c->ttype == 2u)         /* CRUSH: folded, then in eight steps */
            z = (float)(int32_t)(z * 4.0f) * 0.25f;
        if (c->sgain > 0.0f) {           /* SCREECH: its band, resonant, on top */
            float v3 = z - v->sc2, v1 = c->sa1 * v->sc1 + c->sa2 * v3, v2 = v->sc2 + c->sa2 * v->sc1 + c->sa3 * v3;
            v->sc1 = 2.0f * v1 - v->sc1;
            v->sc2 = 2.0f * v2 - v->sc2;
            z = brk_clampf(z + v1 * c->sgain, -1.5f, 1.5f);
        }
        v->tlp += (z - v->tlp) * c->tfz;
        a = v->tenv * c->tsus;              /* (the fold follows the kick down: its punch and its tail kept, not a wall) */
        z = brk_clampf(v->tlo * 1.2f + v->tlp * 0.85f * (a < 1.0f ? a : 1.0f), -1.0f, 1.0f);
        out += (z - out) * c->tmix;
    }
    if (c->t2mix > 0.0f) {               /* LOAF BREAKER: TERROR 2: the whole kick driven into a sine folder (round
                                          * and round: the screech), its mids pushed, clipped hard; the sub back under */
        float a = brk_absf(out), z, m;
        v->t2env += (a - v->t2env) * (a > v->t2env ? 0.05f : 0.0012f);
        v->t2lo += (out - v->t2lo) * c->t2sub;   /* (~110 Hz) */
        z = brk_sin_turns(out * c->t2g * 0.25f);   /* (|x| up to 1: a sine; past it, folded back and forth) */
        v->t2lp += (z - v->t2lp) * c->t2scr;   /* (~600 Hz: what's above it pushed, the scream) */
        z = brk_clampf((z + c->t2push * (z - v->t2lp)) * 1.3f, -1.0f, 1.0f);
        m = v->t2env * 2.5f;
        z = brk_clampf(v->t2lo * 1.3f + z * 0.8f * (m < 1.0f ? m : 1.0f), -1.0f, 1.0f);
        out += (z - out) * c->t2mix;
    }
    if (c->rum > 0.0f && rum_buf && c->rlen) {   /* (SYNTH: RUMBLE, the kick under ~150 Hz into a feedback line, its
                                          * echoes low-passed again: the long low roll after each hit) */
        float tap = (float)rum_buf[rum_pos] * (1.0f / 32767.0f), w;
        v->rlp += (out - v->rlp) * 0.02f;
        w = brk_clampf(v->rlp + tap * c->rfb, -1.0f, 1.0f);
        rum_buf[rum_pos] = (int16_t)(w * 32767.0f);
        if (++rum_pos >= c->rlen)
            rum_pos = 0u;
        v->rout += (tap - v->rout) * 0.06f;
        v->rlvl = brk_absf(v->rout) > v->rlvl ? brk_absf(v->rout) : v->rlvl * 0.9995f;
        out = brk_clampf(out + v->rout * c->rum * 1.5f, -1.2f, 1.2f);
    }
    out *= c->vol;
    v->last = out;
    out += v->declick;
    v->declick *= 0.985f;
    return out;
}

BRK_COLD void kick_init(void)
{
    uint32_t i;
    for (i = 0; i < KICK_LAYERS; i++)
        kv.env[i] = 0.0f;
    kv.declick = kv.last = 0.0f;
    kv.rng = 0x5eed1u;
    kv.tlo = kv.tlp = kv.tenv = 0.0f;
    kv.t2lo = kv.t2lp = kv.t2env = 0.0f;
    kv.sc1 = kv.sc2 = 0.0f;
    kv.rlp = kv.rout = kv.rlvl = kv.dc = 0.0f;
    kv.hold = 0u;
    kv.pm = 1.0f;
    ks_last = 0.0f;
    ks_on = 0u;
    hits_seen = p_hits;
}

BRK_COLD void kick_set(const kick_t *k) { *(kick_t *)&p_k = *k; }
void kick_hit(void) { p_hits++; }
void kick_speed(const float *sp) { ks_sp = sp; }
static const float *ks_pm;               /* (crossbreed) the next render's hits' pitches (kick_pitches), 0 none */
void kick_pitches(const float *pm) { ks_pm = pm; }

void kick_render(float *out, uint32_t n, const uint16_t *at, uint32_t nat)
{
    kick_t k = *(const kick_t *)&p_k;
    kc_t c;
    uint32_t i, j = 0;
    if (p_hits != hits_seen) {           /* a MIDI note: now */
        hits_seen = p_hits;
        kv_hit(&kv);
        kv.pm = 1.0f;
    } else if (!nat && kv.env[KL_SUB] < 3e-5f && kv.env[KL_PUNCH] < 3e-5f && kv.env[KL_GRIT] < 3e-5f &&
               kv.env[KL_CLICK] < 3e-5f && brk_absf(kv.declick) < 1e-5f && kv.rlvl < 1e-4f) {   /* (and the rumble
                                          * died away) */
        for (i = 0; i < n; i++)          /* gone quiet, nothing coming: no work */
            out[i] = 0.0f;
        kv.last = ks_last = 0.0f;
        ks_on = 0u;
        return;
    }
    kc_make(&k, &c);
    if (ks_sp && !ks_on) {               /* the vinyl begins: from the kick as it is */
        ks_a = ks_b = ks_last;
        ks_ph = 0.0f;
    }
    ks_on = ks_sp != 0;
    for (i = 0; i < n; i++) {
        while (j < nat && at[j] <= i) {  /* the steps' kicks, on their samples */
            kv_hit(&kv);
            kv.pm = ks_pm ? ks_pm[j] : 1.0f;   /* (crossbreed: this step's note) */
            j++;
        }
        if (!ks_sp)
            out[i] = kv_tick(&kv, &c);
        else {                           /* the vinyl: the kick's own time runs at the speed, read between */
            float s = ks_sp[i];
            ks_ph += s;
            if (ks_ph >= 1.0f) {
                ks_ph -= 1.0f;
                ks_a = ks_b;
                ks_b = kv_tick(&kv, &c);
            }
            out[i] = (ks_a + (ks_b - ks_a) * ks_ph) * (s < 0.02f ? s * 50.0f : 1.0f);
        }
    }
    ks_last = out[n - 1u];
}

BRK_COLD void kick_shape(const kick_t *k, uint8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    kc_t c;
    kv_t v;
    uint32_t i, total = ms * (BRK_SR / 1000u), per = total / (n ? n : 1u);
    kc_make(k, &c);
    c.rum = 0.0f;                        /* (the picture: no rumble, its line is the kick's playing) */
    v.rng = 0x5eed1u;
    v.sc1 = v.sc2 = 0.0f;
    v.rlp = v.rout = v.rlvl = v.dc = 0.0f;
    v.tlo = v.tlp = v.tenv = 0.0f;
    v.t2lo = v.t2lp = v.t2env = 0.0f;
    v.pm = 1.0f;
    v.last = v.declick = 0.0f;
    kv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t s;
        float pk = 0.0f;
        for (s = 0; s < per; s++) {
            float a = brk_absf(kv_tick(&v, &c));
            if (a > pk)
                pk = a;
        }
        px[i] = (uint8_t)(brk_clampf(pk, 0.0f, 1.0f) * (float)height + 0.5f);
    }
}

/* the built-in kicks: per layer level, pitch, decay, tone; then intensity */
static const float PRESET[KICK_PRESETS][KICK_LAYERS * 4 + 1] = {
    {0.80f, 0.35f, 0.45f, 0.10f, 0.70f, 0.50f, 0.30f, 0.40f, 0.00f, 0.30f, 0.20f, 0.30f, 0.35f, 0.50f, 0.25f, 0.30f,
     0.00f},                             /* 909: clean, round */
    {0.90f, 0.25f, 0.55f, 0.20f, 0.60f, 0.40f, 0.25f, 0.30f, 0.10f, 0.25f, 0.20f, 0.20f, 0.30f, 0.60f, 0.20f, 0.50f,
     0.15f},                             /* TECHNO: deep, a touch of grit */
    {0.75f, 0.45f, 0.40f, 0.40f, 0.80f, 0.55f, 0.35f, 0.60f, 0.35f, 0.35f, 0.30f, 0.50f, 0.40f, 0.55f, 0.20f, 0.40f,
     0.45f},                             /* EARLY HC: punchy, driven */
    {0.90f, 0.30f, 0.60f, 0.60f, 0.70f, 0.45f, 0.45f, 0.70f, 0.55f, 0.30f, 0.55f, 0.70f, 0.35f, 0.60f, 0.20f, 0.50f,
     0.85f},                             /* ROTTERDAM: the long distorted tail */
    {0.70f, 0.30f, 0.50f, 0.50f, 0.60f, 0.60f, 0.30f, 0.50f, 0.80f, 0.55f, 0.45f, 0.90f, 0.50f, 0.70f, 0.30f, 0.80f,
     0.70f},                             /* INDUSTRIAL: metal and noise */
    {0.85f, 0.35f, 0.55f, 0.80f, 0.85f, 0.50f, 0.40f, 0.80f, 0.70f, 0.40f, 0.50f, 0.80f, 0.40f, 0.55f, 0.25f, 0.45f,
     1.00f},                             /* DISTORTO: everything into the clip */
};

BRK_COLD void kick_preset(kick_t *k, uint32_t i)
{
    const float *p = PRESET[i % KICK_PRESETS];
    uint32_t l;
    for (l = 0; l < KICK_LAYERS; l++) {
        k->l[l].level = p[l * 4u];
        k->l[l].pitch = p[l * 4u + 1u];
        k->l[l].decay = p[l * 4u + 2u];
        k->l[l].tone = p[l * 4u + 3u];
        k->l[l].fx = KFX_OFF;
        k->l[l].rsv[0] = k->l[l].rsv[1] = k->l[l].rsv[2] = 0u;
        k->l[l].fa = 0.5f;
        k->l[l].fb = 0.5f;
        k->l[l].wet = 1.0f;
    }
    k->intensity = p[KICK_LAYERS * 4u];
}

BRK_COLD void kick_defaults(kick_t *k)
{
    kick_preset(k, 3u);                  /* ROTTERDAM: CHOPPAH is for the hard stuff */
    k->volume = 0.8f;
}
