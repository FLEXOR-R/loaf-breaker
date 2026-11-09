/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the SNARE / HAT page (ENV). (Part of app.c's unity: included there in order.) */

/* ---- the SNARE / HAT page (ENV): ALGORITHM the voice, ENV again SOUND or SHAPE, PRESETS its presets */
static const char *const PERC_KNOB[PERC_VOICES][2][4] = {
    {{"LEVEL", "TUNE", "DECAY", "TONE"}, {"SNAP", "DRIVE", "PRESET", ""}},
    {{"LEVEL", "TUNE", "DECAY", "METAL/NZ"}, {"CUT", "DRIVE", "PRESET", ""}},
    {{"LEVEL", "TUNE", "DECAY", "TONE"}, {"SPREAD", "DRIVE", "PRESET", ""}},
    {{"LEVEL", "TUNE", "DECAY", "TONE"}, {"BEND", "DRIVE", "PRESET", ""}}};   /* (LOAF BREAKER: the HOOVER's) */   /* (LOAF BREAKER: the clap's; SHAPE's K3
                                          * PRESET, the memory back to a built-in sound) */

/* LOAF BREAKER: the snare's, the hat's and the clap's memories, as the kick's (kick_mem_turn): SNARE 1..10, the one
 * playing kept as it is, the next as it was left (PRESETS on EDIT, SELECT on the drum's SEQ lane) */
static void perc_mem_turn(uint32_t v, int32_t d)
{
    uint8_t *s = perc_slot(v);
    char t[4];
    v %= PERC_VOICES;
    *perc_mem(v, *s) = *perc_voice(v);
    *s = (uint8_t)((*s % PERC_MEMS + (d > 0 ? 1u : PERC_MEMS - 1u)) % PERC_MEMS);
    *perc_voice(v) = *perc_mem(v, *s);
    int_text((int32_t)*s + 1, t);
    app_say(PERC_VOICE_NAME[v], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed();
}
static float *perc_param(int i)
{
    perc_voice_t *v = perc_voice(ui.pvoice);
    float *P[6] = {&v->level, &v->tune, &v->decay, &v->tone, &v->snap, &v->drive};
    int k = ui.ppage ? 4 + i : i;
    return k < 6 ? P[k] : 0;
}
static void perc_value(int i, char *t)
{
    float *p = perc_param(i);
    t[0] = 0;
    if (ui.ppage && i == 2) {            /* PRESET: the built-in sound it last put back */
        text_cat(t, PERC_PRESET_NAME[ui.pvoice % PERC_VOICES][ui.ppreset[ui.pvoice % PERC_VOICES] % PERC_PRESETS]);
        return;
    }
    if (!p)
        return;
    if (!ui.ppage && i == 1)
        hz_text(perc_tune_hz(ui.pvoice, *p), t);
    else if (!ui.ppage && i == 2) {
        int_text((int32_t)(perc_decay_ms(ui.pvoice, *p) + 0.5f), t);
        text_cat(t, " MS");
    } else
        pct_text(*p, t);
}
static void perc_turn(int i, int32_t d)
{
    float *p = perc_param(i);
    char t[12];
    if (ui.ppage && i == 2) {            /* PRESET: the memory back to a built-in sound, by name (no undo) */
        uint8_t *pr = &ui.ppreset[ui.pvoice % PERC_VOICES];
        *pr = (uint8_t)((*pr + (d > 0 ? 1u : PERC_PRESETS - 1u)) % PERC_PRESETS);
        perc_preset(perc_voice(ui.pvoice), ui.pvoice, *pr);
        perc_value(i, t);
        app_say("PRESET", t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed();
        return;
    }
    if (!p)
        return;
    *p = knob_f(*p, d, 0.02f, 0.0f, 1.0f);
    perc_value(i, t);
    app_say(PERC_KNOB[ui.pvoice % PERC_VOICES][ui.ppage][i], t);
    ui.redraw |= RG_BLOCKS;
    changed();
}

/* LOAF BREAKER: the drums' big view (SELECT turned on the KICK menu's big view: the kick, then the snare, the hat, the
 * clap, the hoover, each in it; SEL + EDIT: the classic view, as ever): as the
 * KICK menu, the drum's hit drawn large over its level (its window ~1.4 times its decay, the decay marked in amber), its
 * name, memory and tune; the five drums along the bottom, the one edited lit (stopped: their keys 1-5 play them); under
 * it the part's knobs as cards */
#define PW_W 232u
static uint8_t pw_px[PW_W];
static int8_t pw_wave[PW_W];
static uint32_t pw_sig = 1u, pw_ms = 300u;
static float perc_knob_f(int i)          /* the part's knob i, 0..1 (its card's bar) */
{
    float *p = perc_param(i);
    if (ui.ppage && i == 2)
        return (float)(ui.ppreset[ui.pvoice % PERC_VOICES] % PERC_PRESETS) / (float)(PERC_PRESETS - 1u);
    return p ? *p : 0.0f;
}
static void perc_picture(const brk_status_t *st, int32_t top)
{
    const perc_voice_t *v = perc_voice(ui.pvoice);
    uint32_t i;
    float dms = perc_decay_ms(ui.pvoice, v->decay);
    int32_t dx = 4 + (int32_t)(dms / (float)pw_ms * (float)PW_W);
    char t[24], u[8];
    cv_oy = -top;
    cv_rect(4, 50, PW_W, 1, C_LINE);     /* (the middle) */
    for (i = 0; i < PW_W; i++) {         /* the hit: its level (dim, mirrored), its wave over it */
        int32_t h = pw_px[i], w = pw_wave[i];
        if (h)
            cv_rect(4 + (int32_t)i, 50 - h, 1, 2 * h + 1, C_BLOCK);
        if (w > 0)
            cv_rect(4 + (int32_t)i, 50 - w, 1, w, C_HI);
        else if (w < 0)
            cv_rect(4 + (int32_t)i, 51, 1, -w, C_HI);
    }
    if (dx < 4 + (int32_t)PW_W) {        /* its DECAY, where it has gone (-40 dB) */
        int32_t y;
        for (y = 18; y < 84; y += 4)
            cv_rect(dx, y, 1, 2, C_AMB);
        int_text((int32_t)(dms + 0.5f), t);
        text_cat(t, " MS");
        cv_text(dx + 3 < 200 ? dx + 3 : dx - 3 - text_w(&FONT_XS, t), 72, &FONT_XS, t, C_AMB);
    }
    t[0] = 0;                            /* SNARE 3, its tune */
    text_cat(t, PERC_VOICE_NAME[ui.pvoice % PERC_VOICES]);
    int_text((int32_t)*perc_slot(ui.pvoice) + 1, u);
    text_cat(t, " ");
    text_cat(t, u);
    cv_text(6, 2, &FONT_B, t, C_WHITE);
    t[0] = 0;
    hz_text(perc_tune_hz(ui.pvoice, v->tune), t);
    cv_text(234 - text_w(&FONT_XS, t), 4, &FONT_XS, t, C_AMB);
    for (i = 0; i < NDRUMS; i++) {       /* the five drums, the one edited lit (stopped: key i + 1 plays it) */
        int32_t x = 4 + (int32_t)i * 47;
        int sel = i == ui.einst, hit = ui.aud_d == i && plat_ms() - ui.aud_t < 150u;
        cv_rect(x, 87, 44, 12, hit ? C_HI : sel ? C_BLOCK_HI : C_BLOCK);
        cv_text(x + 3, 88, &FONT_T, i == DR_STAB ? "HOOV" : DRUM_NAME[i], hit ? C_BLACK : sel ? C_WHITE : C_GRAY);
        if (!st->playing) {
            char kn[2] = {(char)('1' + i), 0};
            cv_text(x + 41 - text_w(&FONT_T, kn), 88, &FONT_T, kn, hit ? C_BLACK : C_DIM);
        }
    }
    cv_oy = 0;
}
static void perc_cards(void)             /* the part's knobs: its name, its value large, its bar */
{
    uint32_t i;
    char t[16];
    cv_begin(240, 48, C_BLACK);
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        const char *n = PERC_KNOB[ui.pvoice % PERC_VOICES][ui.ppage & 1u][i];
        cv_rect(x, 0, 56, 46, C_BLOCK);
        if (!n[0])
            continue;
        cv_text(x + 4, 3, &FONT_XS, n, C_GRAY);
        perc_value((int)i, t);
        cv_text(x + 4, 16, text_w(&FONT_B, t) <= 50 ? &FONT_B : &FONT_XS, t, C_WHITE);
        cv_rect(x + 4, 38, 48, 3, C_BLACK);
        cv_rect(x + 4, 38, (int32_t)(48.0f * brk_clampf(perc_knob_f((int)i), 0.0f, 1.0f) + 0.5f), 3, C_HI);
    }
    cv_blit(0, 144);
}
static void draw_perc_big(const brk_status_t *st)
{
    const perc_voice_t *v = perc_voice(ui.pvoice);
    uint32_t sig = sig_of(v, sizeof *v) ^ (ui.pvoice * 2654435761u), top;
    if (sig != pw_sig) {                 /* (its hit again, only when it changed) */
        uint32_t pk = 0u;
        float d = perc_decay_ms(ui.pvoice, v->decay) * 1.4f;
        pw_sig = sig;
        pw_ms = (uint32_t)brk_clampf(d, 60.0f, 900.0f);
        perc_shape(ui.pvoice, v, pw_px, PW_W, pw_ms, 34u);
        perc_wave(ui.pvoice, v, pw_wave, PW_W, pw_ms, 34u);
        for (top = 0; top < PW_W; top++)
            pk = pw_px[top] > pk ? pw_px[top] : pk;
        for (top = 0; pk && top < PW_W; top++) {   /* (as tall as it goes) */
            pw_px[top] = (uint8_t)((uint32_t)pw_px[top] * 34u / pk);
            pw_wave[top] = (int8_t)((int32_t)pw_wave[top] * 34 / (int32_t)pk);
        }
    }
    for (top = 0; top < 100u; top += 60u) {   /* (y 44..143, strips of 60 rows) */
        uint32_t h = 100u - top < 60u ? 100u - top : 60u;
        cv_begin(240, h, C_BLACK);
        perc_picture(st, (int32_t)top);
        cv_blit(0, 44u + top);
    }
    perc_cards();
}

/* the SNARE / HAT page (EDIT): the voice's six settings as bars (the part's lit); then the three drums (LOAF BREAKER:
 * the classic view; the big one: draw_perc_big) */
static void draw_perc(const brk_status_t *st)
{
    if (ui.pbig) {
        draw_perc_big(st);
        return;
    }
    const perc_voice_t *v = perc_voice(ui.pvoice);
    const float P[6] = {v->level, v->tune, v->decay, v->tone, v->snap, v->drive};
    uint32_t i;
    char t[24];
    cv_begin(240, 52, C_BLACK);
    t[0] = 0;
    text_cat(t, PERC_VOICE_NAME[ui.pvoice % PERC_VOICES]);
    {                                    /* (LOAF BREAKER: its memory, "CLAP 3") */
        char u[4];
        int_text((int32_t)*perc_slot(ui.pvoice) + 1, u);
        text_cat(t, " ");
        text_cat(t, u);
    }
    text_cat(t, ui.ppage ? "  SHAPE" : "  SOUND");
    cv_text(6, 0, &FONT_XS, t, C_GRAY);
    for (i = 0; i < 6u; i++) {
        int32_t x = 6 + (int32_t)(i % 4u) * 58, y = 16 + (int32_t)(i / 4u) * 18;
        int lit = (i >= 4u) == (ui.ppage != 0u);
        cv_text(x, y, &FONT_XS, PERC_KNOB[ui.pvoice % PERC_VOICES][i / 4u][i % 4u], lit ? C_GRAY : C_DIM);
        cv_rect(x, y + 13, 52, 3, C_BLOCK);
        cv_rect(x, y + 13, (int32_t)(52.0f * P[i] + 0.5f), 3, lit ? C_HI : C_DIM);
    }
    cv_blit(0, 88);
    draw_drums(st);
}
