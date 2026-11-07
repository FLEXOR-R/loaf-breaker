/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the SNARE / HAT page (ENV). (Part of app.c's unity: included there in order.) */

/* ---- the SNARE / HAT page (ENV): ALGORITHM the voice, ENV again SOUND or SHAPE, PRESETS its presets */
static const char *const PERC_KNOB[PERC_VOICES][2][4] = {
    {{"LEVEL", "TUNE", "DECAY", "TONE"}, {"SNAP", "DRIVE", "PRESET", ""}},
    {{"LEVEL", "TUNE", "DECAY", "METAL/NZ"}, {"CUT", "DRIVE", "PRESET", ""}},
    {{"LEVEL", "TUNE", "DECAY", "TONE"}, {"SPREAD", "DRIVE", "PRESET", ""}},
    {{"LEVEL", "TUNE", "DECAY", "TONE"}, {"CHORD", "DRIVE", "PRESET", ""}}};   /* (LOAF BREAKER: the rave stab's) */   /* (LOAF BREAKER: the clap's; SHAPE's K3
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

/* the SNARE / HAT page (EDIT): the voice's six settings as bars (the part's lit); then the three drums */
static void draw_perc(const brk_status_t *st)
{
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
