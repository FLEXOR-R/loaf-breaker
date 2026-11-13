/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the OVERLAY page (ARP, PRD B-52; LOAF BREAKER: gone 0.1xx-0.243 for FILL, back in 0.244, FLEXOR: "the screen
 * from before with all the FX"). Each white key an effect over everything that plays (dsp/ovl.c), on while held, any
 * together; with LATCH a press switches its effect on and off (and latched ones stay on on the other pages). K1 the
 * amount of the effect touched last, K2 the rate of the ones on the beat, K3 the sweeps' length, K4 LATCH. The ARP
 * black key shares the effect touched last (0.9.27, ovl_last), and the white keys of the effects on are lit (seq.c
 * keys_lit). (Part of app.c's unity: included there in order.) */

static uint32_t ovl_held, ovl_latched;   /* bit OV_*: held down on this page; switched on (LATCH) */
#define ovl_last (set.ovl_pad & 15u)     /* the effect touched last: K1 its amount (0.9.27, FLEXOR: the ARP key's own, one
                                          * effect for both: touched on the page, the key plays it; turned on the key's K1,
                                          * the page tunes it) */

static uint32_t ovl_pad_bits(void) { return ovl_pad_on() ? 1u << (set.ovl_pad & 15u) : 0u; }   /* the ARP key's */
static void ovl_push(void)
{
    ovl_set(ovl_held | ovl_latched | ovl_pad_bits(), set.ovl_amt, set.ovl_rate, set.ovl_sweep);
}
static void ovl_pad_key(int down)        /* GLOUPI BREAKS: the ARP black key down or up */
{
    if (set.ovl_pad & 16u) {             /* LOAF BREAKER: K4 LATCH on: each press turns it on, or off */
        if (down)
            ovlpad_lat ^= 1u;
        ovlpad_held = 0u;
    } else {                             /* (off: on while held) */
        ovlpad_held = (uint8_t)(down != 0);
        ovlpad_lat = 0u;
    }
    ovl_push();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}

static void ovl_keys(uint32_t keys, uint32_t kdn)   /* every input: the white keys on the page, then the engine */
{
    uint32_t i, held = 0u;
    if (ui.page == PG_OVL && ui.opage == 0u)   /* (ARP EFFECTS: the keys; BLACK KEYS, MORE: none) */
        for (i = 0; i < 16u; i++) {
            if (kdn >> WK[i] & 1u) {
                set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | i);
                set_dirty = 1u;
                tut_ovl++;               /* (LOAF BREAKER: the TUTORIAL's count) */
                if (set.ovl_latch)
                    ovl_latched ^= 1u << i;
                app_say(OVL_NAME[i], set.ovl_latch ? (ovl_latched >> i & 1u ? "ON" : "OFF") : "");
                ui.redraw |= RG_BLOCKS | RG_FOOT;   /* (K1: its amount) */
            }
            if (!set.ovl_latch && keys >> WK[i] & 1u)
                held |= 1u << i;
        }
    if (held != ovl_held)
        ui.redraw |= RG_BLOCKS;
    ovl_held = held;
    ovl_push();
}
/* ------------------------------------------------ BLACK KEYS (LOAF BREAKER) --- */
/* LOAF BREAKER (11 Nov 2026, FLEXOR): ARP's second tab, EXPERT's: the black keys' layout. K1 the PRESET (CHOPPAH,
 * FLEXOR LAYOUT, CUSTOM), K2 the KEY (or a black key pressed: it picks itself, nothing played), K3 its EFFECT (any of
 * CHOPPAH's and FLEXOR LAYOUT's, BKF_*; on a preset, K3 makes a CUSTOM of it first). (MORE, a third: gone for
 * now, 0.259, FLEXOR) */
static void bk_release_all(void)         /* (the layout changes: whatever a key had on lets go) */
{
    uint32_t i;
    for (i = 0; i < BRK_NPADS; i++)
        brk_pad((int)i, 0);
    roll_held = 0xFFu;
    roll_sel = 0u;
    kroll = 0u;
    half_lat = 0u;
    pad_latched = 0u;
    ovlpad_held = ovlpad_lat = 0u;
    ovl_push();
    kick_hold = 0u;
    ui.pad_focus = -1;
    ui.kil_focus = 0u;
}
static void bk_say(void)
{
    char t[24] = "KEY ";
    int_text((int32_t)ui.bk_sel % 11 + 1, t + 4);
    app_say(t, BKF_LONG[bk_fn(ui.bk_sel)]);
}
static void bk_pick(uint32_t j)          /* a black key pressed on BLACK KEYS */
{
    ui.bk_sel = (uint8_t)(j % 11u);
    bk_say();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void bk_value(int i, char *t)
{
    t[0] = 0;
    if (i == 0)
        text_cat(t, bk_preset() == BKP_FLEXOR ? "FLEXOR" : BKP_NAME[bk_preset()]);   /* (the foot's room) */
    else if (i == 1)
        int_text((int32_t)ui.bk_sel % 11 + 1, t);
    else if (i == 2)
        text_cat(t, BKF_LONG[bk_fn(ui.bk_sel)]);
}
static void bk_turn(int i, int32_t d)
{
    int dir = d > 0 ? 1 : -1;
    uint32_t j;
    if (set.novice)
        return;
    if (i == 0) {
        uint32_t p = (uint32_t)clamp((int32_t)bk_preset() + dir, 0, BKP_N - 1);
        if (p == bk_preset())
            return;
        bk_release_all();
        set.bkeys.bk[1] = (uint8_t)p;
        app_say("BLACK KEYS", BKP_NAME[p]);
    } else if (i == 1) {
        ui.bk_sel = (uint8_t)clamp((int32_t)ui.bk_sel + dir, 0, 10);
        bk_say();
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    } else if (i == 2) {
        uint32_t f = (bk_fn(ui.bk_sel) + (uint32_t)(dir > 0 ? 1 : BKF_N - 1)) % BKF_N;
        if (bk_preset() != BKP_CUSTOM) { /* (a preset: its copy, CUSTOM, then the key) */
            for (j = 0; j < 11u; j++)
                set.bkeys.bk[2u + j] = (uint8_t)bk_fn(j);
            set.bkeys.bk[1] = BKP_CUSTOM;
        }
        bk_release_all();
        set.bkeys.bk[2u + ui.bk_sel % 11u] = (uint8_t)f;
        bk_say();
    } else
        return;
    set_dirty = 1u;
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* the BLACK KEYS tab: the eleven keys as the FM-1 has them (3, 2, 3, 2, 1), each with its function, the one edited
 * lit; under them that key and its effect, the preset */
static void draw_bk(void)
{
    static const uint8_t GRP[11] = {0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 4};
    uint32_t band, j, p = bk_preset();
    char t[24];
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        cv_oy = -(int32_t)band * 52;
        for (j = 0; j < 11u; j++) {
            uint32_t f = bk_fn(j);
            int32_t x = 4 + (int32_t)j * 19 + (int32_t)GRP[j] * 6;
            int sel = j == ui.bk_sel % 11u, chop = f == BK_LAY[0][j];
            cv_rect(x, 6, 17, 30, sel ? C_WHITE : chop ? C_LINE : C_AMB);
            cv_rect(x + 1, 7, 15, 28, sel ? C_HI : C_BLOCK);
            t[0] = 0;
            int_text((int32_t)j + 1, t);
            cv_text(x + (17 - text_w(&FONT_T, t)) / 2, 9, &FONT_T, t, sel ? C_BLACK : C_DIM);
            cv_text(x + (17 - text_w(&FONT_T, BKF_NAME[f])) / 2, 23, &FONT_T, BKF_NAME[f], sel ? C_BLACK : C_WHITE);
        }
        t[0] = 0;
        text_cat(t, "KEY ");
        int_text((int32_t)ui.bk_sel % 11 + 1, t + 4);
        cv_text(6, 40, &FONT_XS, t, C_GRAY);
        cv_text(6, 50, &FONT_B, BKF_LONG[bk_fn(ui.bk_sel)], C_HI);
        cv_text(6, 72, &FONT_XS, "PRESET", C_GRAY);
        cv_text(234 - text_w(&FONT_B, BKP_NAME[p]), 68, &FONT_B, BKP_NAME[p], p == BKP_CUSTOM ? C_AMB : C_WHITE);
        cv_text(6, 86, &FONT_XS, p == BKP_CUSTOM ? "A BLACK KEY PICKS ITSELF" : "K3: A CUSTOM COPY, THAT KEY CHANGED",
                C_DIM);
        cv_oy = 0;
        cv_blit(0, 88u + band * 52u);
    }
}

static void ovl_value(int i, char *t)
{
    t[0] = 0;
    if (ui.opage == 1u) {
        bk_value(i, t);
        return;
    }
    if (ui.opage) {
        t[0] = 0;
        return;
    }
    if (i == 0) {
        int_text(set.ovl_amt[ovl_last & 15u], t);
        text_cat(t, " %");
    } else if (i == 1)
        text_cat(t, OVL_RATE_NAME[set.ovl_rate & 3u]);
    else if (i == 2)
        text_cat(t, OVL_SWEEP_NAME[set.ovl_sweep & 3u]);
    else
        text_cat(t, set.ovl_latch ? "ON" : "OFF");
}
static void ovl_turn(int i, int32_t d)
{
    char t[16];
    if (ui.opage == 1u) {
        bk_turn(i, d);
        return;
    }
    if (ui.opage)
        return;
    if (i == 0) {
        uint8_t *a = &set.ovl_amt[ovl_last & 15u];
        *a = (uint8_t)clamp((int32_t)*a + d * 2, 0, 100);
    } else if (i == 1)
        set.ovl_rate = (uint8_t)clamp((int32_t)set.ovl_rate + (d > 0 ? 1 : -1), 0, 3);
    else if (i == 2)
        set.ovl_sweep = (uint8_t)clamp((int32_t)set.ovl_sweep + (d > 0 ? 1 : -1), 0, 3);
    else {
        set.ovl_latch = d > 0 ? 1u : 0u;
        if (!set.ovl_latch)
            ovl_latched = 0u;            /* (back to held: what was latched lets go) */
    }
    changed();
    ovl_push();
    ovl_value(i, t);
    app_say(i == 0 ? OVL_NAME[ovl_last & 15u] : i == 1 ? "RATE" : i == 2 ? "SWEEP" : "LATCH", t);
    ui.redraw |= RG_BLOCKS;
}

/* the OVERLAY page's strip: sixteen cards in key order, four by four; one sounding lit, one latched outlined amber,
 * the one K1 tunes outlined white; each with a bar of its amount and its key */
static void draw_ovl(void)
{
    uint32_t band, i, on = ovl_sounding() | ovl_held | ovl_latched | ovl_pad_bits();
    char t[4];
    if (ui.opage == 1u) {
        draw_bk();
        return;
    }
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        for (i = band * 8u; i < band * 8u + 8u; i++) {
            int32_t x = 1 + (int32_t)(i % 4u) * 60, y = (int32_t)((i / 4u) & 1u) * 26;
            int lit = on >> i & 1u, lat = ovl_latched >> i & 1u, sel = i == ovl_last;
            cv_rect(x, y, 57, 24, lit ? C_HI : lat ? C_AMB : sel ? C_WHITE : C_LINE);
            cv_rect(x + 1, y + 1, 55, 22, lit ? C_HI : C_BLOCK);
            t[0] = 0;
            int_text((int32_t)i + 1, t);
            cv_text(x + 4, y + 1, &FONT_XS, OVL_NAME[i], lit ? C_BLACK : sel ? C_HI : C_GRAY);
            cv_text(x + 54 - text_w(&FONT_T, t), y + 12, &FONT_T, t, lit ? C_BLACK : C_DIM);   /* its key */
            cv_rect(x + 4, y + 18, 34, 2, lit ? C_BLOCK_HI : C_LINE);                         /* its amount */
            cv_rect(x + 4, y + 18, (int32_t)set.ovl_amt[i] * 34 / 100, 2, lit ? C_BLACK : lat ? C_AMB : C_GRAY);
        }
        cv_blit(0, 88u + band * 52u);
    }
}
