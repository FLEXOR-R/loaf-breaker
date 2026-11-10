/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's snare and hi-hat (perc.h). */
#include "perc.h"
#include "brk.h"
#include "brk_math.h"

const char *const PERC_VOICE_NAME[PERC_VOICES] = {"SNARE", "HAT", "CLAP", "MELODY"};
const char *const MEL_NAME[MEL_N] = {"ACID", "LEAD", "PAD", "REESE"};
const char *const PERC_PRESET_NAME[PERC_VOICES][PERC_PRESETS] = {{"909", "TIGHT", "FAT", "GABBER"},
                                                                 {"CLOSED", "808", "OPEN", "TRASH"},
                                                                 {"808", "909", "TIGHT", "BIG"},
                                                                 {"FACTORY", "DARK", "BRIGHT", "DIRTY"}};   /* (LOAF BREAKER: the
                                          * MELODY's: its instrument's own sound, then three turns of it) */

static inline float soft(float x) { return x / (1.0f + brk_absf(x)); }
static float decay_factor(float ms) { return brk_exp2f_c(-6.644f / (ms * 0.001f * (float)BRK_SR)); }   /* -40 dB */

static float onepole(float hz) { return 1.0f - brk_exp2f_c(-9.0647f * hz / (float)BRK_SR); }   /* 1 - e^(-2 pi f / fs) */

static const float HAT_RATIO[6] = {1.0f, 1.4827f, 1.8003f, 2.5460f, 2.6303f, 3.8967f};   /* 808: 205.3 .. 800 Hz */

static volatile uint32_t p_inst;        /* LOAF BREAKER: the MELODY's instrument (perc_mel) */
static volatile float p_mvol = 0.8f, p_step = 4000.0f;
BRK_COLD float perc_tune_hz(uint32_t voice, float t)
{
    t = brk_clampf(t, 0.0f, 1.0f);
    if (voice == PV_STAB)                /* (LOAF BREAKER: the MELODY's root: 55 .. 220 Hz; REESE an octave down) */
        return (p_inst == MEL_REESE ? 27.5f : 55.0f) * brk_exp2f_c(t * 2.0f);
    return voice == PV_SNARE ? 120.0f * brk_exp2f_c(t * 1.737f) : voice == PV_CLAP ? 700.0f * brk_exp2f_c(t * 1.8f)
           : 100.0f * brk_exp2f_c(t * 3.0f);
}
BRK_COLD float perc_decay_ms(uint32_t voice, float d)
{
    d = brk_clampf(d, 0.0f, 1.0f);
    if (voice == PV_STAB)                /* (the MELODY: ACID its filter's fall 40 .. 640 ms, LEAD its release 30 .. 630,
                                          * PAD its swell 20 .. 1220, REESE its movement's period 0.5 .. 8.5 s) */
        return p_inst == MEL_ACID ? 40.0f + 600.0f * d * d : p_inst == MEL_LEAD ? 30.0f + 600.0f * d * d
               : p_inst == MEL_PAD ? 20.0f + 1200.0f * d * d : 500.0f + 8000.0f * d * d;
    return voice == PV_SNARE ? 60.0f + 640.0f * d * d : voice == PV_CLAP ? 60.0f + 840.0f * d * d
           : 15.0f + 885.0f * d * d;
}

typedef struct {                         /* a voice sounding */
    float env, env2, pe, ph[6], hp1, hp2, lp, last, declick;
    float pm;                            /* (the stab) this hit's pitch, a factor */
    float mph[9], mf, mft, mgk, mgate, mamp, mfe, ic1, ic2, lp2, lfo;   /* (LOAF BREAKER: the MELODY's) */
    uint8_t macc;
} pv_t;
typedef struct {                         /* a voice's settings, mapped for the block */
    float f, dec, dec2, lvl, hpk, lpk, body, noise, crack, g, make;
    float att, rel, res, cut, det, ch[3];   /* (LOAF BREAKER: the MELODY's) */
    uint32_t inst;
} pc_t;

static volatile perc_t p_p;
static volatile uint32_t p_hits[PERC_VOICES];
static uint32_t hits_seen[PERC_VOICES];
static pv_t pv[PERC_VOICES];
static uint32_t rng = 0x2545F491u;

static inline float noise(void)
{
    rng = rng * 1664525u + 1013904223u;
    return (float)(int32_t)rng * (1.0f / 2147483648.0f);
}

BRK_COLD static void pc_make(uint32_t voice, const perc_voice_t *s, float volume, pc_t *c)
{
    float snap = brk_clampf(s->snap, 0.0f, 1.0f), tone = brk_clampf(s->tone, 0.0f, 1.0f);
    float dr = brk_clampf(s->drive, 0.0f, 1.0f);
    float ms = perc_decay_ms(voice, s->decay);
    c->f = perc_tune_hz(voice, s->tune) / (float)BRK_SR;
    if (voice == PV_STAB)                /* (LOAF BREAKER: the MELODY: its own fader) */
        volume = p_mvol;
    c->lvl = brk_clampf(s->level, 0.0f, 1.0f) * brk_clampf(volume, 0.0f, 1.0f);
    c->g = 1.0f + 15.0f * dr * dr;       /* the drive, made back up to a full-scale peak */
    c->make = (1.0f + c->g) / c->g;
    if (voice == PV_SNARE) {
        c->dec = decay_factor(ms * 0.55f);           /* the body: shorter than the noise */
        c->dec2 = decay_factor(ms);
        c->hpk = onepole(700.0f * brk_exp2f_c(tone * 2.2f));
        c->lpk = onepole(3500.0f * brk_exp2f_c(tone * 1.5f));
        c->body = 1.0f - 0.6f * snap;
        c->noise = 0.35f + 0.85f * snap;
        c->crack = snap;
    } else if (voice == PV_STAB) {       /* LOAF BREAKER: the MELODY, its instrument's own (pc_mel) */
        uint32_t in = p_inst % MEL_N;
        c->inst = in;
        c->att = 1.0f - decay_factor(4.0f);   /* (an attack of a few ms) */
        c->rel = decay_factor(in == MEL_ACID ? 12.0f : in == MEL_REESE ? 70.0f : ms);
        c->dec = decay_factor(ms);
        if (in == MEL_ACID) {            /* one saw, a resonant low-pass that falls from each note (DECAY), its
                                          * cut-off (TONE), its resonance (SNAP) */
            c->cut = 70.0f * brk_exp2f_c(tone * 5.5f);
            c->res = 0.45f + 0.5f * snap;
        } else if (in == MEL_LEAD) {     /* seven saws spread (SNAP the spread), one an octave down, a soft top (TONE) */
            c->det = 0.001f + 0.022f * snap;
            c->lpk = onepole(1200.0f * brk_exp2f_c(tone * 3.3f));
        } else if (in == MEL_PAD) {      /* a chord of three, three saws each, swelling in and out (DECAY); SNAP the
                                          * chord: minor, major, power */
            static const float CH[3][3] = {{1.0f, 1.18921f, 1.49831f}, {1.0f, 1.25992f, 1.49831f},
                                           {1.0f, 1.49831f, 2.0f}};
            uint32_t q = snap < 0.34f ? 0u : snap < 0.67f ? 1u : 2u, k;
            for (k = 0; k < 3u; k++)
                c->ch[k] = CH[q][k];
            c->att = 1.0f - c->dec;
            c->rel = decay_factor(ms * 1.5f + 60.0f);
            c->lpk = onepole(500.0f * brk_exp2f_c(tone * 3.6f));
        } else {                         /* REESE: two saws rubbing (SNAP how far apart), a filter breathing (DECAY its
                                          * period, TONE where) */
            c->det = 0.002f + 0.02f * snap;
            c->cut = 110.0f * brk_exp2f_c(tone * 4.2f);
            c->res = 0.5f;
            c->dec2 = 1.0f / (ms * 0.001f * (float)BRK_SR);   /* (its movement: a turn per period) */
        }
    } else if (voice == PV_CLAP) {       /* LOAF BREAKER: the bursts' band, their spacing, the tail */
        float hz = c->f * (float)BRK_SR;
        c->dec = decay_factor(ms);
        c->dec2 = decay_factor(14.0f);   /* a burst: gone in 14 ms */
        c->hpk = onepole(hz * 0.6f);
        c->lpk = onepole(hz * (1.2f + 1.5f * tone));
        c->body = 0.0f;
        c->crack = (4.0f + 14.0f * snap) * 0.001f * (float)BRK_SR;   /* samples between bursts */
        c->noise = 2.5f;
    } else {
        c->dec = c->dec2 = decay_factor(ms);
        c->hpk = onepole(3000.0f * brk_exp2f_c(snap * 1.74f));
        c->lpk = 0.0f;
        c->body = 1.0f - tone;           /* metal against noise */
        c->noise = tone;
        c->crack = 0.0f;
    }
}

static void pv_hit(pv_t *v)
{
    uint32_t k;
    v->declick = v->last;                /* what still sounds fades out under the new hit */
    v->env = v->env2 = v->pe = 1.0f;
    for (k = 0; k < 6u; k++)
        v->ph[k] = 0.0f;
}

static inline float snare_tick(pv_t *v, const pc_t *c)
{
    float b, n = noise(), x;
    b = 0.6f * brk_sin_turns(v->ph[0]) + 0.4f * brk_sin_turns(v->ph[1]);
    v->ph[0] += c->f * (1.0f + 0.7f * v->pe);
    v->ph[1] += c->f * 1.62f * (1.0f + 0.7f * v->pe);
    if (v->ph[0] >= 1.0f)
        v->ph[0] -= 1.0f;
    if (v->ph[1] >= 1.0f)
        v->ph[1] -= 1.0f;
    v->pe *= 0.9985f;                    /* (the body's drop: ~15 ms) */
    v->hp1 += (n - v->hp1) * c->hpk;     /* noise: a high-pass then a low-pass, a band */
    v->lp += ((n - v->hp1) - v->lp) * c->lpk;
    x = b * v->env * c->body + v->lp * v->env2 * (c->noise + c->crack * 2.0f * v->pe * v->pe);
    v->env *= c->dec;
    v->env2 *= c->dec2;
    return x;
}

static inline float hat_tick(pv_t *v, const pc_t *c)
{
    float m = 0.0f, x, h;
    uint32_t k;
    for (k = 0; k < 6u; k++) {           /* six squares */
        v->ph[k] += c->f * HAT_RATIO[k] * 2.0f;
        if (v->ph[k] >= 1.0f)
            v->ph[k] -= 1.0f;
        m += v->ph[k] < 0.5f ? 1.0f : -1.0f;
    }
    x = m * (1.0f / 6.0f) * c->body + noise() * c->noise;
    v->hp1 += (x - v->hp1) * c->hpk;     /* two high-passes at CUT */
    h = x - v->hp1;
    v->hp2 += (h - v->hp2) * c->hpk;
    x = (h - v->hp2) * 2.5f * v->env;
    v->env *= c->dec;
    return x;
}

/* LOAF BREAKER: the clap: three more bursts c->crack samples apart (ph[0] counting, ph[1] how many), then the tail */
static inline float clap_tick(pv_t *v, const pc_t *c)
{
    float n = noise(), x;
    v->hp1 += (n - v->hp1) * c->hpk;
    v->lp += ((n - v->hp1) - v->lp) * c->lpk;
    if (v->ph[1] < 3.0f) {
        v->ph[0] += 1.0f;
        if (v->ph[0] >= c->crack) {
            v->ph[0] = 0.0f;
            v->ph[1] += 1.0f;
            v->pe = 0.7f;                /* (the first burst the loudest) */
        }
        x = v->lp * v->pe;
    } else {                             /* the tail under the last burst */
        float t = 0.5f * v->env;
        x = v->lp * (v->pe > t ? v->pe : t);
        v->env *= c->dec;
    }
    v->pe *= c->dec2;
    return x * c->noise;
}

/* LOAF BREAKER: the MELODY (in the HOOVER's place): a note holds its gate (how long, the app's: to the next note),
 * glides in when it follows the note before (the step before: SLIDE; LEAD, PAD and REESE while one still sounds) */
static inline float msaw(float *ph, float f)
{
    *ph += f;
    if (*ph >= 1.0f)
        *ph -= 1.0f;
    return 2.0f * *ph - 1.0f;
}
static inline float svf_lp(pv_t *v, float x, float cut, float res)   /* a resonant low-pass (cut in Hz) */
{
    float g = 3.14159f * cut * (1.0f / (float)BRK_SR), a1, v1, v2;
    g = g > 0.9f ? 0.9f : g;
    a1 = 1.0f / (1.0f + g * (g + 2.0f - 2.0f * res));
    v1 = a1 * v->ic1 + g * a1 * (x - v->ic2);
    v2 = v->ic2 + g * v1;
    v->ic1 = 2.0f * v1 - v->ic1;
    v->ic2 = 2.0f * v2 - v->ic2;
    return v2;
}
static const float LEADD[7] = {-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f};
static inline float stab_tick(pv_t *v, const pc_t *c)
{
    float f, s = 0.0f;
    uint32_t i;
    v->mf += (v->mft - v->mf) * v->mgk;  /* (the glide) */
    if (v->mgate > 0.0f) {
        v->mgate -= 1.0f;
        v->mamp += (1.0f - v->mamp) * c->att;
    } else
        v->mamp *= c->rel;
    f = v->mf;
    if (c->inst == MEL_ACID) {
        float cut = c->cut * (1.0f + (5.0f + 4.0f * (float)v->macc) * v->mfe);
        v->mfe *= c->dec;
        s = svf_lp(v, msaw(&v->mph[0], f), cut, c->res + 0.04f * (float)v->macc) * 1.4f;
    } else if (c->inst == MEL_LEAD) {
        for (i = 0; i < 7u; i++)
            s += msaw(&v->mph[i], f * (1.0f + LEADD[i] * c->det));
        s = s * 0.22f + 0.4f * msaw(&v->mph[7], f * 0.5f);
        v->lp += (s - v->lp) * c->lpk;
        s = v->lp;
    } else if (c->inst == MEL_PAD) {
        for (i = 0; i < 9u; i++)
            s += msaw(&v->mph[i], f * c->ch[i / 3u] * (1.0f + (float)((int32_t)(i % 3u) - 1) * 0.004f));
        s *= 0.16f;
        v->lp += (s - v->lp) * c->lpk;
        v->lp2 += (v->lp - v->lp2) * c->lpk;
        s = v->lp2;
    } else {
        float m;
        v->lfo += c->dec2;
        if (v->lfo >= 1.0f)
            v->lfo -= 1.0f;
        m = v->lfo < 0.5f ? 4.0f * v->lfo - 1.0f : 3.0f - 4.0f * v->lfo;   /* (a triangle) */
        s = msaw(&v->mph[0], f * (1.0f + c->det)) + msaw(&v->mph[1], f * (1.0f - c->det));
        s = svf_lp(v, s * 0.6f, c->cut * brk_exp2f(1.2f * m), c->res);
    }
    return s * v->mamp;
}
static void mel_hit(pv_t *v, const pc_t *c, float pm, float gate, uint32_t fl)   /* a note of the MELODY */
{
    float step = p_step, ft = c->f * pm;
    int legato = v->mamp > 0.05f && (c->inst != MEL_ACID || (fl & MEL_SLIDE));
    if (gate <= 0.0f)
        gate = step * 0.5f;
    if (c->inst == MEL_ACID)             /* (ACID: short, unless the next step holds a note) */
        gate = fl & MEL_TIE ? gate * 1.05f : (gate < step * 0.55f ? gate : step * 0.55f);
    v->mft = ft;
    v->mgate = gate;
    if (legato && c->inst != MEL_PAD)    /* (into it, from the note sounding: ~40 ms) */
        v->mgk = 1.0f - decay_factor(40.0f);
    else {
        v->mf = ft;
        v->mgk = 1.0f;
        if (c->inst == MEL_ACID || !legato)
            v->mfe = 1.0f;
    }
    v->macc = (uint8_t)(fl & MEL_ACCENT ? 1u : 0u);
}

static inline float voice_tick(uint32_t k, pv_t *v, const pc_t *c)
{
    return k == PV_SNARE ? snare_tick(v, c) : k == PV_CLAP ? clap_tick(v, c) : k == PV_STAB ? stab_tick(v, c)
           : hat_tick(v, c);
}

BRK_COLD void perc_init(void)
{
    uint32_t k;
    for (k = 0; k < PERC_VOICES; k++) {
        pv[k].env = pv[k].env2 = pv[k].pe = 0.0f;
        pv[k].last = pv[k].declick = 0.0f;
        pv[k].ph[1] = 3.0f;              /* (LOAF BREAKER: the clap's bursts done: none until a hit) */
        pv[k].pm = 1.0f;
        pv[k].mf = pv[k].mft = pv[k].mgate = pv[k].mamp = pv[k].mfe = 0.0f;   /* (LOAF BREAKER: the MELODY quiet) */
        pv[k].ic1 = pv[k].ic2 = pv[k].lp2 = pv[k].lfo = 0.0f;
        pv[k].mgk = 1.0f;
        hits_seen[k] = p_hits[k];
    }
}
BRK_COLD void perc_set(const perc_t *p) { *(perc_t *)&p_p = *p; }
static volatile float p_live_pm[PERC_VOICES] = {1.0f, 1.0f, 1.0f, 1.0f};   /* LOAF BREAKER: a live hit's pitch */
void perc_hit(uint32_t voice) { perc_hit_pm(voice, 1.0f); }
void perc_hit_pm(uint32_t voice, float pm)
{
    p_live_pm[voice % PERC_VOICES] = pm > 0.0f ? pm : 1.0f;
    p_hits[voice % PERC_VOICES]++;
}

static const float *vs_sp;               /* LOAF BREAKER: the vinyl's speed (perc_speed), as the kick's */
static float vs_ph, vs_a, vs_b, vs_last;
static uint8_t vs_on;
void perc_speed(const float *sp) { vs_sp = sp; }

static const float *vs_pm;               /* LOAF BREAKER: the stab's hits' pitches (perc_pitches), 0 none */
void perc_pitches(const float *pm) { vs_pm = pm; }
static const float *vs_gate;             /* LOAF BREAKER: the MELODY's notes: how long, their flags (perc_notes) */
static const uint8_t *vs_fl;
void perc_notes(const float *gate, const uint8_t *fl) { vs_gate = gate; vs_fl = fl; }
void perc_mel(uint32_t inst, float volume, float step)
{
    p_inst = inst % MEL_N;
    p_mvol = volume;
    p_step = step > 100.0f ? step : 100.0f;
}

static uint8_t live_mel;                 /* LOAF BREAKER: a MELODY note by hand (perc_hit_pm), at this render */
void perc_render(float *out, uint32_t n, const uint16_t *at, const uint8_t *who, uint32_t nat)
{
    perc_t s = *(const perc_t *)&p_p;
    pc_t c[PERC_VOICES];
    uint32_t i, j, k, hit = nat ? 1u : 0u;
    for (k = 0; k < PERC_VOICES; k++)
        if (p_hits[k] != hits_seen[k]) {     /* a MIDI note: now */
            hits_seen[k] = p_hits[k];
            if (k == PV_STAB)            /* (LOAF BREAKER: the MELODY: a note, a beat long) */
                live_mel = 1u;
            else
                pv_hit(&pv[k]);
            pv[k].pm = p_live_pm[k];
            hit = 1u;
        }
    if (!hit && pv[0].env < 3e-5f && pv[0].env2 < 3e-5f && pv[1].env < 3e-5f && pv[2].env < 3e-5f &&
        pv[2].pe < 3e-5f && pv[3].mamp < 3e-5f && pv[3].mgate <= 0.0f && brk_absf(pv[0].declick) + brk_absf(pv[1].declick) +
        brk_absf(pv[2].declick) + brk_absf(pv[3].declick) < 1e-5f) {
        for (i = 0; i < n; i++)          /* quiet, nothing coming: no work */
            out[i] = 0.0f;
        pv[0].last = pv[1].last = pv[2].last = pv[3].last = vs_last = 0.0f;
        vs_on = 0u;
        return;
    }
    for (k = 0; k < PERC_VOICES; k++)
        pc_make(k, &s.v[k], s.volume, &c[k]);
    if (live_mel) {
        mel_hit(&pv[PV_STAB], &c[PV_STAB], pv[PV_STAB].pm, p_step * 4.0f, 0u);
        live_mel = 0u;
    }
    if (vs_sp && !vs_on) {               /* the vinyl begins: from the drums as they are */
        vs_a = vs_b = vs_last;
        vs_ph = 0.0f;
    }
    vs_on = vs_sp != 0;
    for (i = 0, j = 0; i < n; i++) {
        float y = 0.0f, f = 1.0f;
        while (j < nat && at[j] <= i) {
            for (k = 0; k < PERC_VOICES; k++)
                if ((who[j] >> k) & 1u) {
                    if (k == PV_STAB)    /* (LOAF BREAKER: the MELODY: this step's note, how long, its flags) */
                        mel_hit(&pv[k], &c[k], vs_pm ? vs_pm[j] : 1.0f, vs_gate ? vs_gate[j] : 0.0f, vs_fl ? vs_fl[j] : 0u);
                    else
                        pv_hit(&pv[k]);
                    pv[k].pm = k == PV_STAB && vs_pm ? vs_pm[j] : 1.0f;
                }
            j++;
        }
        if (vs_sp) {                     /* the vinyl: their own time at the speed, read between two samples */
            f = vs_sp[i] < 0.02f ? vs_sp[i] * 50.0f : 1.0f;
            vs_ph += vs_sp[i];
            if (vs_ph < 1.0f) {
                out[i] = (vs_a + (vs_b - vs_a) * vs_ph) * f;
                continue;
            }
            vs_ph -= 1.0f;
        }
        for (k = 0; k < PERC_VOICES; k++) {
            pv_t *v = &pv[k];
            float x = voice_tick(k, v, &c[k]);
            x = soft(x * c[k].g) * c[k].make * c[k].lvl;
            v->last = x;
            y += x + v->declick;
            v->declick *= 0.985f;
        }
        if (vs_sp) {
            vs_a = vs_b;
            vs_b = y;
            y = (vs_a + (vs_b - vs_a) * vs_ph) * f;
        }
        out[i] = y;
    }
    vs_last = out[n - 1u];
}

BRK_COLD void perc_shape(uint32_t voice, const perc_voice_t *s, uint8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    pc_t c;
    pv_t v;
    uint32_t i, k, per = ms * (BRK_SR / 1000u) / (n ? n : 1u);
    voice %= PERC_VOICES;
    pc_make(voice, s, 1.0f, &c);
    for (k = 0; k < 6u; k++)
        v.ph[k] = 0.0f;
    v.hp1 = v.hp2 = v.lp = v.last = v.declick = 0.0f;
    v.pm = 1.0f;
    pv_hit(&v);
    v.declick = 0.0f;
    if (voice == PV_STAB) {              /* (LOAF BREAKER: the MELODY: a note held a third of the window) */
        for (k = 0; k < 9u; k++)
            v.mph[k] = 0.0f;
        v.mf = v.mft = v.mgate = v.mamp = v.mfe = v.ic1 = v.ic2 = v.lp2 = v.lfo = 0.0f;
        mel_hit(&v, &c, 1.0f, (float)(ms * (BRK_SR / 1000u)) * 0.35f, 0u);
    }
    for (i = 0; i < n; i++) {
        uint32_t j;
        float pk = 0.0f;
        for (j = 0; j < per; j++) {
            float x = voice_tick(voice, &v, &c);
            x = brk_absf(soft(x * c.g) * c.make * c.lvl);
            if (x > pk)
                pk = x;
        }
        px[i] = (uint8_t)(brk_clampf(pk, 0.0f, 1.0f) * (float)height + 0.5f);
    }
}

BRK_COLD void perc_wave(uint32_t voice, const perc_voice_t *s, int8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    pc_t c;
    pv_t v;
    uint32_t i, k, per = ms * (BRK_SR / 1000u) / (n ? n : 1u);
    voice %= PERC_VOICES;
    pc_make(voice, s, 1.0f, &c);
    for (k = 0; k < 6u; k++)
        v.ph[k] = 0.0f;
    v.hp1 = v.hp2 = v.lp = v.last = v.declick = 0.0f;
    v.pm = 1.0f;
    pv_hit(&v);
    v.declick = 0.0f;
    if (voice == PV_STAB) {              /* (LOAF BREAKER: the MELODY: a note held a third of the window) */
        for (k = 0; k < 9u; k++)
            v.mph[k] = 0.0f;
        v.mf = v.mft = v.mgate = v.mamp = v.mfe = v.ic1 = v.ic2 = v.lp2 = v.lfo = 0.0f;
        mel_hit(&v, &c, 1.0f, (float)(ms * (BRK_SR / 1000u)) * 0.35f, 0u);
    }
    for (i = 0; i < n; i++) {
        uint32_t j;
        float best = 0.0f;
        for (j = 0; j < per; j++) {
            float x = soft(voice_tick(voice, &v, &c) * c.g) * c.make * c.lvl;
            if (brk_absf(x) > brk_absf(best))
                best = x;
        }
        px[i] = (int8_t)(brk_clampf(best, -1.0f, 1.0f) * (float)height);
    }
}

/* the built-in sounds: level, tune, decay, tone, snap (the hat's cut), drive */
static const float PRESET[PERC_VOICES][PERC_PRESETS][6] = {
    {{0.80f, 0.45f, 0.40f, 0.50f, 0.55f, 0.10f},    /* 909 */
     {0.75f, 0.65f, 0.20f, 0.70f, 0.70f, 0.20f},    /* TIGHT */
     {0.85f, 0.25f, 0.65f, 0.35f, 0.45f, 0.30f},    /* FAT */
     {0.85f, 0.55f, 0.45f, 0.60f, 0.75f, 0.85f}},   /* GABBER */
    {{0.60f, 0.55f, 0.10f, 0.55f, 0.55f, 0.00f},    /* CLOSED */
     {0.60f, 0.35f, 0.18f, 0.20f, 0.45f, 0.10f},    /* 808 */
     {0.55f, 0.50f, 0.75f, 0.50f, 0.50f, 0.05f},    /* OPEN */
     {0.65f, 0.80f, 0.15f, 0.85f, 0.70f, 0.80f}},   /* TRASH */
    {{0.75f, 0.35f, 0.45f, 0.40f, 0.45f, 0.10f},    /* CLAP 808 (snap: the bursts' spread) */
     {0.75f, 0.55f, 0.35f, 0.60f, 0.35f, 0.15f},    /* 909 */
     {0.70f, 0.60f, 0.15f, 0.55f, 0.20f, 0.10f},    /* TIGHT */
     {0.80f, 0.30f, 0.65f, 0.35f, 0.70f, 0.30f}},   /* BIG */
    {{0.0f}},                                      /* (the MELODY's: MEL_DEF, by instrument) */
};
/* LOAF BREAKER: each MELODY instrument's own sound (level, tune, decay, tone, snap, drive) */
static const float MEL_DEF[MEL_N][6] = {
    {0.60f, 0.625f, 0.45f, 0.35f, 0.80f, 0.45f},    /* ACID: C3 (tune 0.625: 130.8 Hz; REESE C2), a quick fall */
    {0.55f, 0.625f, 0.25f, 0.65f, 0.45f, 0.35f},    /* LEAD */
    {0.50f, 0.625f, 0.55f, 0.45f, 0.10f, 0.15f},    /* PAD: minor */
    {0.65f, 0.625f, 0.40f, 0.40f, 0.35f, 0.40f},    /* REESE */
};
BRK_COLD void perc_mel_default(perc_voice_t *v, uint32_t inst)
{
    const float *p = MEL_DEF[inst % MEL_N];
    v->level = p[0];
    v->tune = p[1];
    v->decay = p[2];
    v->tone = p[3];
    v->snap = p[4];
    v->drive = p[5];
}

BRK_COLD void perc_preset(perc_voice_t *v, uint32_t voice, uint32_t i)
{
    const float *p = PRESET[voice % PERC_VOICES][i % PERC_PRESETS];
    if (voice == PV_STAB) {              /* (LOAF BREAKER: the MELODY: its instrument's own, then darker, brighter,
                                          * dirtier) */
        perc_mel_default(v, p_inst);
        i %= PERC_PRESETS;
        v->tone = brk_clampf(v->tone + (i == 1u ? -0.25f : i == 2u ? 0.25f : 0.0f), 0.0f, 1.0f);
        v->drive = brk_clampf(v->drive + (i == 3u ? 0.4f : 0.0f), 0.0f, 1.0f);
        return;
    }
    v->level = p[0];
    v->tune = p[1];
    v->decay = p[2];
    v->tone = p[3];
    v->snap = p[4];
    v->drive = p[5];
}

BRK_COLD void perc_defaults(perc_t *p)
{
    uint32_t k;
    for (k = 0; k < PERC_VOICES; k++)
        perc_preset(&p->v[k], k, 0u);
    p->volume = 0.8f;
}
