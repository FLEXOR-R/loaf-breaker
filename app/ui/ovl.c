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

static uint32_t ovl_pad_bits(void)       /* the ARP keys' effects on (0.9.45: each key its own) */
{
    uint32_t j, m = 0u;
    for (j = 0; j < 11u; j++)
        if (arpk_on(j))
            m |= 1u << arpk_fx(j);
    return m;
}
static uint32_t sw_fx_on;                /* (0.9.50: SEL WHITE's EFFECT keys held, their effects, a bit each OV_*) */
static void ovl_push(void)
{
    uint32_t on = ovl_held | ovl_latched | ovl_pad_bits() | sw_fx_on;
    ovl_set(on, set.ovl_amt, set.ovl_rate, set.ovl_sweep);
    kick_ovl = (uint8_t)(on >> OV_KX2 & 7u);   /* (0.9.55, FLEXOR: KICK X2, X4, TRI: the render's, as the black keys) */
}
static void ovl_pad_key(uint32_t j, int down)   /* GLOUPI BREAKS: the ARP black key j (0..10) down or up */
{
    uint16_t b = (uint16_t)(1u << (j % 11u));
    if (down) {                          /* (0.9.45: the key touched last: its knobs; the OVERLAY page's K1 its effect) */
        arp_foc = (uint8_t)(j % 11u);
        set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | arpk_fx(j));
    }
    if (arpk_latch(j)) {                 /* LOAF BREAKER: K4 LATCH on: each press turns it on, or off */
        if (down)
            arp_lat ^= b;
        arp_held &= (uint16_t)~b;
    } else {                             /* (off: on while held) */
        arp_held = (uint16_t)(down ? arp_held | b : arp_held & ~b);
        arp_lat &= (uint16_t)~b;
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
                arpk_set_fx(arpk_focus(), i);   /* (0.9.45: the ARP key touched last plays it) */
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
    arp_held = arp_lat = 0u;
    ovl_push();
    kick_hold = 0u;
    ui.pad_focus = -1;
    ui.kil_focus = 0u;
    ui.x2_focus = 0u;
    kick_x2 = 0u;
}
static void bk_say(void)
{
    char t[24] = "KEY ";
    int_text((int32_t)ui.bk_sel % 11 + 1, t + 4);
    if (ui.opage == 2u) {                /* (0.9.49: SEL KEYS, SEL + the key) */
        text_cat(t, " + SEL");
        app_say(t, selb_fn(ui.bk_sel) ? BKF_LONG[selb_fn(ui.bk_sel) - 1u] : "ITS OWN");
        return;
    }
    app_say(t, BKF_LONG[bk_fn(ui.bk_sel)]);
}
static void bk_pick(uint32_t j)          /* a black key pressed on BLACK KEYS */
{
    ui.bk_sel = (uint8_t)(j % 11u);
    bk_say();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* LOAF BREAKER (0.9.50, FLEXOR): SEL WHITE, ARP's fourth tab (EXPERT): each white key a shortcut, pressed with SEL held
 * or locked, of one of six kinds, any key any kind: NONE (as without it), KICK 1-20, PATTERN 1-64 (playing: at the
 * loop's end), BREAK (any), EFFECT (an OVERLAY effect while held), DROP (AUTO as SEL + ENV, or into P1-64); kept in
 * OBJ_KX2's room (kx2.rsv: the kind, then the value, two bytes a key) */
enum { SW_NONE, SW_KICK, SW_PAT, SW_BRK, SW_FX, SW_DROP, SW_KIT, SW_N };   /* (0.9.54: KIT) */
static const char *const SW_NAME[SW_N] = {"NONE", "KICK", "PATTERN", "BREAK", "EFFECT", "DROP", "KIT"};
static uint8_t sw_sel;                   /* (the white key edited, 0..15) */
static uint32_t sw_cat(uint32_t i) { uint32_t c = kx2.rsv[2u * (i % 16u)]; return c < SW_N ? c : SW_NONE; }
static uint32_t sw_nval(uint32_t c)      /* (a kind's values) */
{
    return c == SW_KICK ? KICK_MEMS : c == SW_PAT ? NPAT_ALL : c == SW_BRK ? n_breaks : c == SW_FX ? 16u
           : c == SW_DROP ? 1u + NPAT_ALL : c == SW_KIT ? 16u : 1u;
}
static uint32_t sw_val(uint32_t i) { uint32_t n = sw_nval(sw_cat(i)); return kx2.rsv[2u * (i % 16u) + 1u] % (n ? n : 1u); }
static void sw_text(uint32_t i, char *t, int brief)   /* (its shortcut: in full, or in five letters for its card) */
{
    uint32_t c = sw_cat(i), v = sw_val(i);
    t[0] = 0;
    if (c == SW_KICK) {
        text_cat(t, brief ? "K" : "KICK ");
        int_text((int32_t)v + 1, t + (brief ? 1 : 5));
    } else if (c == SW_PAT) {
        text_cat(t, "P");
        int_text((int32_t)v + 1, t + 1);
    } else if (c == SW_BRK) {
        text_cat(t, break_name(v));
        if (brief)
            t[5] = 0;
    } else if (c == SW_FX) {
        text_cat(t, OVL_NAME[v % 16u]);
        if (brief)
            t[5] = 0;
    } else if (c == SW_KIT) {
        text_cat(t, brief ? "KIT" : "KIT ");
        int_text((int32_t)v + 1, t + (brief ? 3 : 4));
    } else if (c == SW_DROP) {
        if (!v)
            text_cat(t, brief ? "DROP" : "DROP AUTO");
        else {
            text_cat(t, brief ? "D" : "DROP > P");
            int_text((int32_t)v, t + (brief ? 1 : 8));
        }
    } else
        text_cat(t, brief ? "-" : "NONE");
}
static void sw_say(void)
{
    char a[16] = "SEL + KEY ", b[24];
    int_text((int32_t)sw_sel + 1, a + 10);
    sw_text(sw_sel, b, 0);
    app_say(a, b);
}
static void sw_pick(uint32_t i)          /* (a white key pressed on SEL WHITE: it picks itself) */
{
    sw_sel = (uint8_t)(i % 16u);
    sw_say();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void sw_value(int i, char *t)
{
    t[0] = 0;
    if (i == 1)
        int_text((int32_t)sw_sel + 1, t);
    else if (i == 2)
        text_cat(t, SW_NAME[sw_cat(sw_sel)]);
    else if (i == 3 && sw_cat(sw_sel) != SW_NONE) {
        char u[24];
        sw_text(sw_sel, u, 0);
        text_cat(t, sw_cat(sw_sel) == SW_DROP ? u + 5 : u);   /* (DROP: AUTO, > P12) */
    }
}
static void sw_turn(int i, int32_t d)
{
    uint8_t *c = &kx2.rsv[2u * sw_sel], *v = c + 1;
    int dir = d > 0 ? 1 : -1;
    if (set.novice)
        return;
    if (i == 1) {
        sw_sel = (uint8_t)clamp((int32_t)sw_sel + dir, 0, 15);
    } else if (i == 2) {                 /* (the kind: its value from the start) */
        *c = (uint8_t)clamp((int32_t)sw_cat(sw_sel) + dir, 0, SW_N - 1);
        *v = 0u;
        set_dirty = 1u;
    } else if (i == 3 && sw_cat(sw_sel) != SW_NONE) {   /* (the value, round; 4 a detent past 8 on the long lists) */
        uint32_t n = sw_nval(sw_cat(sw_sel));
        int32_t step = n > 20u && (d > 2 || d < -2) ? 4 : 1;
        *v = (uint8_t)(((int32_t)sw_val(sw_sel) + dir * step + (int32_t)n * 4) % (int32_t)n);
        set_dirty = 1u;
    } else
        return;
    sw_say();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* SEL WHITE's page: the sixteen white keys as two rows of eight cards, each its shortcut in brief (the kind's colour:
 * a dash none), the one edited lit; under them that key and its shortcut in full */
static void draw_sw(void)
{
    uint32_t band, i;
    char t[24];
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        cv_oy = -(int32_t)band * 52;
        for (i = 0; i < 16u; i++) {
            int32_t x = 1 + (int32_t)(i % 8u) * 30, y = 2 + (int32_t)(i / 8u) * 26;
            int sel = i == sw_sel, has = sw_cat(i) != SW_NONE;
            cv_rect(x, y, 28, 24, sel ? C_WHITE : has ? C_AMB : C_LINE);
            cv_rect(x + 1, y + 1, 26, 22, sel ? C_HI : C_BLOCK);
            t[0] = 0;
            int_text((int32_t)i + 1, t);
            cv_text(x + 2, y + 2, &FONT_T, t, sel ? C_BLACK : C_DIM);
            sw_text(i, t, 1);
            cv_text(x + (28 - text_w(&FONT_T, t)) / 2, y + 12, &FONT_T, t, sel ? C_BLACK : has ? C_WHITE : C_DIM);
        }
        t[0] = 0;
        text_cat(t, "KEY ");
        int_text((int32_t)sw_sel + 1, t + 4);
        text_cat(t, " WITH SEL");
        cv_text(6, 56, &FONT_XS, t, C_GRAY);
        sw_text(sw_sel, t, 0);
        cv_text(6, 66, &FONT_B, t, sw_cat(sw_sel) ? C_HI : C_DIM);
        cv_text(6, 88, &FONT_XS, "K3 THE KIND, K4 ITS VALUE; ANY KEY ANY KIND", C_DIM);
        cv_oy = 0;
        cv_blit(0, 88u + band * 52u);
    }
}
/* LOAF BREAKER (0.9.54, FLEXOR): KITS, ARP's fifth tab (EXPERT): sixteen kits as the ARP effects' cards, each a kick, a
 * break, a tempo and a pattern to go to at once (SEL + a white key given KIT n in SEL WHITE); 0 in a field: KEEP. A
 * white key or ALGORITHM picks the kit, K1 KICK, K2 BREAK, K3 BPM, K4 PATTERN (from KEEP, the first turn right takes
 * what plays now) */
static uint8_t kit_sel;
static int kit_empty(uint32_t k)
{
    k %= 16u;
    return !kx2.kit[k].kick && !kx2.kit[k].brk && !kit_bpm(k) && !kx2.kit[k].pat;
}
static void kit_value(int i, char *t)
{
    uint32_t k = kit_sel % 16u;
    t[0] = 0;
    if (i == 0 && kx2.kit[k].kick) {
        text_cat(t, "KICK ");
        int_text(kx2.kit[k].kick, t + 5);
    } else if (i == 1 && kx2.kit[k].brk && kx2.kit[k].brk <= n_breaks)
        text_cat(t, break_name(kx2.kit[k].brk - 1u));
    else if (i == 2 && kit_bpm(k))
        tenths_text(kit_bpm(k), t);
    else if (i == 3 && kx2.kit[k].pat) {
        t[0] = 'P';
        int_text(kx2.kit[k].pat, t + 1);
    } else
        text_cat(t, "KEEP");
}
static void kit_say(int i)
{
    static const char *const N[4] = {"KICK", "BREAK", "BPM", "PATTERN"};
    char a[24] = "KIT ", t[16];
    int_text((int32_t)kit_sel % 16 + 1, a + 4);
    text_cat(a, " ");
    text_cat(a, N[i & 3]);
    kit_value(i, t);
    app_say(a, t);
}
static void kit_turn(int i, int32_t d)
{
    uint32_t k = kit_sel % 16u;
    int dir = d > 0 ? 1 : -1;
    if (set.novice)
        return;
    if (i < 0) {                         /* (ALGORITHM: the kit, round) */
        kit_sel = (uint8_t)((k + (dir > 0 ? 1u : 15u)) % 16u);
        kit_say(0);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    }
    if (i == 0)
        kx2.kit[k].kick = (uint8_t)(!kx2.kit[k].kick && dir > 0 ? set.kslot % KICK_MEMS + 1u
                                    : (uint32_t)clamp((int32_t)kx2.kit[k].kick + dir, 0, KICK_MEMS));
    else if (i == 1)
        kx2.kit[k].brk = (uint8_t)(!kx2.kit[k].brk && dir > 0 ? set.brk % n_breaks + 1u
                                   : (uint32_t)clamp((int32_t)kx2.kit[k].brk + dir, 0, (int32_t)n_breaks));
    else if (i == 2) {                   /* (1 BPM a detent; 50 .. 300, under 50: KEEP) */
        int32_t b = kit_bpm(k) ? (int32_t)kit_bpm(k) / 10 + d : dir > 0 ? (set.tempo_x10 + 5) / 10 : 0;
        kx2.rsv[KIT_BPM0 + k] = (uint8_t)(b < 50 ? 0 : (b > 300 ? 300 : b) - 49);
    } else if (i == 3)
        kx2.kit[k].pat = (uint8_t)(!kx2.kit[k].pat && dir > 0 ? pat_cur() + 1u
                                   : (uint32_t)clamp((int32_t)kx2.kit[k].pat + dir, 0, (int32_t)NPAT_ALL));
    else
        return;
    set_dirty = 1u;
    kit_say(i);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void kit_pick(uint32_t k)         /* (a white key on KITS: its kit, nothing played) */
{
    kit_sel = (uint8_t)(k % 16u);
    kit_say(0);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* KITS' page: sixteen cards, four by four in key order, each its kit in brief (the kick, the tempo, the pattern; EMPTY
 * when every field keeps), the one edited outlined white, a kit with something lit amber */
static void draw_kits(void)
{
    uint32_t band, i;
    char t[24], u[8];
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        for (i = band * 8u; i < band * 8u + 8u; i++) {
            int32_t x = 1 + (int32_t)(i % 4u) * 60, y = (int32_t)((i / 4u) & 1u) * 26;
            int sel = i == kit_sel % 16u, has = !kit_empty(i);
            cv_rect(x, y, 57, 24, sel ? C_WHITE : has ? C_AMB : C_LINE);
            cv_rect(x + 1, y + 1, 55, 22, sel ? C_HI : C_BLOCK);
            t[0] = 0;
            text_cat(t, "KIT ");
            int_text((int32_t)i + 1, t + 4);
            cv_text(x + 4, y + 1, &FONT_XS, t, sel ? C_BLACK : has ? C_WHITE : C_GRAY);
            t[0] = 0;
            if (!has)
                text_cat(t, "EMPTY");
            else {
                if (kx2.kit[i].kick) {
                    text_cat(t, "K");
                    int_text(kx2.kit[i].kick, u);
                    text_cat(t, u);
                    text_cat(t, " ");
                }
                if (kx2.kit[i].pat) {
                    text_cat(t, "P");
                    int_text(kx2.kit[i].pat, u);
                    text_cat(t, u);
                    text_cat(t, " ");
                }
                if (kit_bpm(i)) {
                    int_text((int32_t)kit_bpm(i) / 10, u);
                    text_cat(t, u);
                }
            }
            cv_text(x + 4, y + 13, &FONT_T, t, sel ? C_BLACK : has ? C_AMB : C_DIM);
        }
        cv_blit(0, 88u + band * 52u);
    }
}
static void bk_value(int i, char *t)
{
    t[0] = 0;
    if (ui.opage == 2u) {                /* (0.9.49: SEL KEYS: K2 the key, K3 its SEL function) */
        if (i == 1)
            int_text((int32_t)ui.bk_sel % 11 + 1, t);
        else if (i == 2)
            text_cat(t, selb_fn(ui.bk_sel) ? BKF_LONG[selb_fn(ui.bk_sel) - 1u] : "ITS OWN");
        return;
    }
    if (i == 0)
        text_cat(t, bk_preset() == BKP_FLEXOR ? "FLEXOR" : BKP_NAME[bk_preset()]);   /* (the foot's room) */
    else if (i == 1)
        int_text((int32_t)ui.bk_sel % 11 + 1, t);
    else if (i == 2)
        text_cat(t, BKF_LONG[bk_fn(ui.bk_sel)]);
    else if (bk_fn(ui.bk_sel) == BKF_ARP)  /* (0.9.45: K4, an ARP key's own effect) */
        text_cat(t, OVL_NAME[arpk_fx(ui.bk_sel)]);
}
static void bk_turn(int i, int32_t d)
{
    int dir = d > 0 ? 1 : -1;
    uint32_t j;
    if (set.novice)
        return;
    if (ui.opage == 2u && i != 1) {      /* (0.9.49, FLEXOR: SEL KEYS: K3 the key's function with SEL, ITS OWN (none)
                                          * then every function, round) */
        uint8_t *v = &kx2.selk[ui.bk_sel % 11u];
        if (i != 2)
            return;
        bk_release_all();
        *v = (uint8_t)((selb_fn(ui.bk_sel) + (uint32_t)(dir > 0 ? 1 : BKF_N)) % (BKF_N + 1u));
        bk_say();
        set_dirty = 1u;
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    }
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
    } else if (bk_fn(ui.bk_sel) == BKF_ARP) {   /* K4 (0.9.45, FLEXOR): an ARP key's own effect, several ARP keys at
                                          * once each with its own */
        uint32_t e = (arpk_fx(ui.bk_sel) + (dir > 0 ? 1u : 15u)) & 15u;
        arpk_set_fx(ui.bk_sel, e);
        if (arpk_focus() == ui.bk_sel % 11u)
            set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | e);
        ovl_push();
        app_say("ARP FX", OVL_NAME[e]);
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
    int sl = ui.opage == 2u;             /* (0.9.49: SEL KEYS) */
    char t[40];
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        cv_oy = -(int32_t)band * 52;
        for (j = 0; j < 11u; j++) {
            uint32_t f = bk_fn(j), sf = selb_fn(j);
            int32_t x = 4 + (int32_t)j * 19 + (int32_t)GRP[j] * 6;
            int sel = j == ui.bk_sel % 11u, chop = sl ? !sf : f == BK_LAY[0][j];
            const char *n = sl ? (sf ? BKF_NAME[sf - 1u] : "-") : BKF_NAME[f];
            cv_rect(x, 6, 17, 30, sel ? C_WHITE : chop ? C_LINE : C_AMB);
            cv_rect(x + 1, 7, 15, 28, sel ? C_HI : C_BLOCK);
            t[0] = 0;
            int_text((int32_t)j + 1, t);
            cv_text(x + (17 - text_w(&FONT_T, t)) / 2, 9, &FONT_T, t, sel ? C_BLACK : C_DIM);
            cv_text(x + (17 - text_w(&FONT_T, n)) / 2, 23, &FONT_T, n, sel ? C_BLACK : sl && !sf ? C_DIM : C_WHITE);
        }
        t[0] = 0;
        text_cat(t, "KEY ");
        int_text((int32_t)ui.bk_sel % 11 + 1, t + 4);
        if (sl) {                        /* (0.9.49, FLEXOR: SEL KEYS: what each key does pressed with SEL) */
            text_cat(t, " WITH SEL");
            cv_text(6, 40, &FONT_XS, t, C_GRAY);
            cv_text(6, 50, &FONT_B, selb_fn(ui.bk_sel) ? BKF_LONG[selb_fn(ui.bk_sel) - 1u] : "ITS OWN", C_HI);
            t[0] = 0;
            text_cat(t, "WITHOUT: ");
            text_cat(t, BKF_LONG[bk_fn(ui.bk_sel)]);
            cv_text(6, 72, &FONT_XS, t, C_GRAY);
            cv_text(6, 86, &FONT_XS, "SEL + THIS KEY PLAYS THE ONE ABOVE", C_DIM);
            cv_oy = 0;
            cv_blit(0, 88u + band * 52u);
            continue;
        }
        cv_text(6, 40, &FONT_XS, t, C_GRAY);
        if (bk_fn(ui.bk_sel) == BKF_ARP) {   /* (0.9.45: an ARP key with its own effect, K4) */
            cv_text(6, 50, &FONT_B, "ARP", C_HI);
            cv_text(6 + text_w(&FONT_B, "ARP "), 50, &FONT_B, OVL_NAME[arpk_fx(ui.bk_sel)], C_WHITE);
        } else
            cv_text(6, 50, &FONT_B, BKF_LONG[bk_fn(ui.bk_sel)], C_HI);
        cv_text(6, 72, &FONT_XS, "PRESET", C_GRAY);
        cv_text(234 - text_w(&FONT_B, BKP_NAME[p]), 68, &FONT_B, BKP_NAME[p], p == BKP_CUSTOM ? C_AMB : C_WHITE);
        cv_text(6, 86, &FONT_XS, p == BKP_CUSTOM ? "A BLACK KEY PICKS ITSELF" : "K3: A CUSTOM COPY, THAT KEY CHANGED",
                C_DIM);
        cv_oy = 0;
        cv_blit(0, 88u + band * 52u);
    }
}

static void ovl_value(int i, char *t);
static void ovl_step(int32_t d)          /* (0.9.52, FLEXOR: ALGORITHM, the effect K1-K3 tune: the next or the one before,
                                          * round, nothing played; the ARP key's too, as a key touched here) */
{
    uint32_t e = (ovl_last + (d > 0 ? 1u : 15u)) & 15u;
    char t[16];
    arpk_set_fx(arpk_focus(), e);
    set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | e);
    set_dirty = 1u;
    ovl_push();
    ovl_value(0, t);
    app_say(OVL_NAME[e], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void ovl_value(int i, char *t)
{
    t[0] = 0;
    if (ui.opage == 4u) {                /* (0.9.54: KITS) */
        kit_value(i, t);
        return;
    }
    if (ui.opage == 3u) {                /* (0.9.50: SEL WHITE) */
        sw_value(i, t);
        return;
    }
    if (ui.opage >= 1u) {                /* (BLACK KEYS; 0.9.49 SEL KEYS) */
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
    } else if (i == 1)                   /* (0.9.50, FLEXOR: K2 this effect's THRU: BREAK, KICK, THRU; K3 its RATE or
                                          * SWEEP, the one it has) */
        text_cat(t, ARP_THRU[arp_route(ovl_last)]);
    else if (i == 2) {
        int tk = ovl_time_kind(ovl_last);
        text_cat(t, tk == 1 ? OVL_RATE_NAME[set.ovl_rate & 3u] : tk == 2 ? OVL_SWEEP_NAME[set.ovl_sweep & 3u] : "-");
    } else
        text_cat(t, set.ovl_latch ? "ON" : "OFF");
}
static void ovl_turn(int i, int32_t d)
{
    char t[16];
    if (ui.opage == 4u) {
        kit_turn(i, d);
        return;
    }
    if (ui.opage == 3u) {
        sw_turn(i, d);
        return;
    }
    if (ui.opage >= 1u) {
        bk_turn(i, d);
        return;
    }
    if (ui.opage)
        return;
    if (i == 0) {
        uint8_t *a = &set.ovl_amt[ovl_last & 15u];
        *a = (uint8_t)clamp((int32_t)*a + d * 2, 0, 100);
    } else if (i == 1) {
        arp_thru_turn(ovl_last, d);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    } else if (i == 2 && ovl_time_kind(ovl_last) == 1)
        set.ovl_rate = (uint8_t)clamp((int32_t)set.ovl_rate + (d > 0 ? 1 : -1), 0, 3);
    else if (i == 2 && ovl_time_kind(ovl_last) == 2)
        set.ovl_sweep = (uint8_t)clamp((int32_t)set.ovl_sweep + (d > 0 ? 1 : -1), 0, 3);
    else if (i == 2)
        return;
    else {
        set.ovl_latch = d > 0 ? 1u : 0u;
        if (!set.ovl_latch)
            ovl_latched = 0u;            /* (back to held: what was latched lets go) */
    }
    changed();
    ovl_push();
    ovl_value(i, t);
    app_say(i == 0 ? OVL_NAME[ovl_last & 15u] : i == 2 ? (ovl_time_kind(ovl_last) == 1 ? "RATE" : "SWEEP") : "LATCH", t);
    ui.redraw |= RG_BLOCKS;
}

/* the OVERLAY page's strip: sixteen cards in key order, four by four; one sounding lit, one latched outlined amber,
 * the one K1 tunes outlined white; each with a bar of its amount and its key */
static void draw_ovl(void)
{
    uint32_t band, i, on = ovl_sounding() | ovl_held | ovl_latched | ovl_pad_bits();
    char t[4];
    if (ui.opage == 4u) {                /* (0.9.54: KITS) */
        draw_kits();
        return;
    }
    if (ui.opage == 3u) {                /* (0.9.50: SEL WHITE) */
        draw_sw();
        return;
    }
    if (ui.opage >= 1u) {                /* (BLACK KEYS; 0.9.49 SEL KEYS) */
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
            if (arp_route(i))            /* (0.9.50: its THRU, BREAK or KICK alone) */
                cv_text(x + 4, y + 12, &FONT_T, arp_route(i) == 1u ? "BREAK" : "KICK", lit ? C_BLACK : C_AMB);
            cv_rect(x + 4, y + 18, 34, 2, lit ? C_BLOCK_HI : C_LINE);                         /* its amount */
            cv_rect(x + 4, y + 18, (int32_t)set.ovl_amt[i] * 34 / 100, 2, lit ? C_BLACK : lat ? C_AMB : C_GRAY);
        }
        cv_blit(0, 88u + band * 52u);
    }
}
