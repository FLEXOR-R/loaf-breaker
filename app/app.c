/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH on fm1-base: the app's unity build (compiled into fm1-base's device/unity.c after the platform, so gfx.c's
 * cv_* drawing and the plat_* calls are in scope) and the glue between the contract (app.h) and the engine
 * (dsp/brk.c, dsp/brk_fx.c: their own -O2 units).
 *
 * M1 scope (PRD 13): the built-in break, 16 slices on the white keys, the 11 pads on the black keys, play / reset,
 * tempo and tap tempo, pitch on OCT-/OCT+, the generator, the four FX and the DJ filter, settings kept by SAVE.
 * Breaks: the bank built into the image (app/bank_data.h, which ./build.sh generates from breaks/breaks.txt), chosen
 * on ALGORITHM; a build with no bank falls back to one synthesised break. */
#include <stddef.h>
#if defined(PLAT_HOST) && !defined(PLAT_WEB)
#include <stdlib.h>                  /* (getenv: LOAF_ASK, the first start's question) */
#endif
#include "adpcm.h"
#include "brk_bank.h"
#include "brk_math.h"
#include "brk_synth.h"
#include "kick.h"
#include "ovl.h"
#include "perc.h"
#if defined(__has_include)
#if __has_include("bank_data.h")
#include "bank_data.h"
#define CH_HAVE_BANK 1
#endif
#endif
#ifndef CH_HAVE_BANK
#define CH_HAVE_BANK 0
#endif
#if CH_LOGO
#include "splash_data.h"            /* LOAF BREAKER: the splash and the title (tools/make_egg.py) */
#include "title_data.h"
#endif

/* ------------------------------------------------------------------ settings --- */
#define SET_MAGIC 0x37504843u            /* "CHP7" (the soundboard gone, the drums and patterns in): another magic
                                          * loads defaults */
/* From CHP7 on the settings only grow at the end: a shorter save (an older firmware's) loads into the front and the
 * fields after it keep their defaults (SET_SIZE_MIN: CHP7's size; an append keeps it). A change that isn't an append
 * needs a new magic (and loses the saves). */
typedef struct { perc_voice_t v[2]; float volume; } perc2_t;   /* LOAF BREAKER: the snare's and the hat's, as saves
                                          * have them (perc_t has every drum now) */
typedef struct {
    uint32_t magic;
    uint16_t tempo_x10;                  /* 400..3000 */
    int8_t pitch;                        /* semitones */
    uint8_t swing;                       /* 50..75 % */
    brk_gen_t gen;
    brk_fx_t fx;
    uint8_t brk;                         /* the break: 0 the synthesised one, 1.. the bank's slots */
    uint8_t clk_in;                      /* MIDI clock in: 0 off, 1 follow (the tempo, start / stop) */
    uint8_t clk_out;                     /* MIDI clock out: 0 off, 1 on (24 a beat while playing, start / stop) */
    uint8_t ble_off;                     /* Bluetooth MIDI: 0 on (the default), 1 off */
    uint8_t midi_to;                     /* where MIDI goes: 0 USB and Bluetooth, 1 USB, 2 Bluetooth */
    uint8_t pitch_keep;                  /* pitch: 1 KEEP LENGTH (granular; the default), 2 SPEED (repitch) */
    uint8_t grain_ms;                    /* its grains, 10..80 ms */
    uint8_t gate;                        /* how much of each slice sounds, 5..100 % (100: off) */
    uint8_t snap;                        /* where a key lands while playing: SNAP_SL index + 1 */
    uint8_t brk_level;                   /* the MIXER's BREAK fader, 0..100 % */
    uint8_t drums_chain;                 /* the drums (kick, snare, hat): 1 through the FX chain, 0 around it */
    uint8_t pattern;                     /* the pattern playing, 0..NPAT-1 (patterns: their own store object) */
    uint8_t bta_on;                      /* Bluetooth audio out (GLOBAL, third part): on at power-on, the last device
                                          * connected again */
    uint8_t bta_codec;                   /* its SBC coding: 0 joint stereo (fm1-base's), 1 plain stereo */
    uint8_t click_gone;                  /* (the metronome's click, gone with TAP TEMPO: its byte kept) */
    uint8_t rsv;
    brk_padset_t pads;                   /* the pads' own settings (K1-K4 while a pad is held) */
    brk_keyset_t keys[BRK_KEYS];         /* each white key's loop: length, pitch, start, mode (K1-K4 while held) */
    brk_slicefx_t slx[BRK_KEYS];         /* each slice's own effect and level (a key held, FX: its other layer) */
    kick_t kick;                         /* the kick (PRD K-*): its sound and level */
    perc2_t perc;                        /* the snare and the hat (PRD K-10): their sounds and level (LOAF BREAKER:
                                          * the others in pmore) */
    uint8_t rsv2[8];                     /* (v0.52's fills, gone; their bytes kept: older saves still load) */
    /* (appended 3 Nov 2026: the overlay effects, ARP's page, PRD B-52) */
    uint8_t ovl_amt[16];                 /* each white key's effect's amount, 0..100 (OV_*) */
    uint8_t ovl_rate;                    /* CHOP's rate (GATE, WAH, PAN follow it): 0 1/4 .. 3 1/32 */
    uint8_t ovl_sweep;                   /* DIVE's and RISE's sweep: 0 1/4 bar .. 3 2 bars */
    uint8_t ovl_latch;                   /* 1: a white key switches its effect on and off (0: on while held) */
    uint8_t ovl_rsv;
    /* (appended 4 Nov 2026: MB COMP's detail per FX slot, PRD A-49a; it moves with its slot) */
    brk_mbc_t mbc[BRK_FX_SLOTS];
    /* (appended 4 Nov 2026: every slot's extras, its MORE and MOD pages, PRD A-49b; they move with their slot) */
    brk_fxx_t fxx[BRK_FX_SLOTS];
    /* (appended: GLOUPI BREAKS' skin, GLOBAL's MORE page; 0 RED, an older save's too) */
    uint8_t skin;
    int8_t kick_dj;                      /* GLOUPI BREAKS: HOME's KICK FLT (SEQ's K2 too), LOAF BREAKER: 0 off .. 100
                                          * high-pass (an older save's low-pass, below 0: off) */
    uint8_t half_mt_off;                 /* GLOUPI BREAKS: HALF SPEED's K2, bit 0 MASTER TEMPO off (0, older saves: on);
                                          * LOAF BREAKER: bit 1 its K3 LATCH (a press on, a press off); bit 2 KIL's
                                          * LATCH (KIL held, K1) */
    uint8_t ovl_pad;                     /* GLOUPI BREAKS: the ARP black key, bits 0-3 its OVERLAY effect (OV_*),
                                          * bit 4 its LATCH (gone), bit 5 OFF (K3: the effects browsed silent),
                                          * bits 6-7 THRU (K2): 0 BOTH, 1 BREAK (the kick around the chain and
                                          * the overlays), 2 KICK (the overlays on the kick alone) */
    /* (appended: LOAF BREAKER's MORE page; 0, older saves: off) */
    uint8_t auto_save;                   /* K2 AUTO SAVE: 1, a minute after a change, saved once stopped */
    uint8_t vinyl;                       /* the VINYL BREAK key's THRU (K3): 0 BOTH (older saves too), 1 BREAK, 2 KICK
                                          * (the kick, the snare and the hat) */
    uint8_t kslot;                       /* the kick memory playing, 0..19 (KICK 1..20, 0.9.2): set.kick is it */
    uint8_t duck;                        /* LOAF BREAKER (crossbreed): DUCK, the break down under each kick, 0 off .. 100
                                          * (SEL + K1) */
    /* (appended: LOAF BREAKER's kick memories; an older save's: the built-in kicks, its own kick KICK 1; one of six
     * kept them, KICK 7..10 begin anew) */
#define KICK_MEMS 20u                    /* KICK 1..20 (0.9.2, FLEXOR: ten more): 1..10 in the settings, the steps and
                                          * the KICK LAB's objects as before, 11..20 in their own (km2, OBJ_KMEM2); reach
                                          * them through kmem_at, ksmem_at, kx_at, kxh_at, klmem_at */
#define KMEM_A 10u                       /* (the first ten's arrays: their saves' layout kept) */
#define KX_N 9u                          /* (each kick memory's own knobs: steps.kx) */
    kick_t kmem[KMEM_A];                 /* KICK 1..10, each as left (PRESETS on KICK, SELECT on SEQ: nothing lost) */
    /* (appended: LOAF BREAKER's drums; an older save's: their first presets) */
    perc_voice_t pmore[1];               /* the CLAP's sound (PV_CLAP; the STAB's: pstab) */
    /* (appended: LOAF BREAKER's snare, hat and clap memories, as the kick's; an older save's: their presets, its own
     * sound each one's first) */
#define PERC_MEMS 10u
    uint8_t pslot[3];                    /* the memory playing, PV_SNARE..PV_CLAP 0..9 (SNARE 1..10...): perc_voice() is
                                          * it (the stab's: sslot) */
    uint8_t drop_brk;                    /* LOAF BREAKER (crossbreed): the break a drop brings in, 1.. (0: the same one;
                                          * MORE's K4) */
    perc_voice_t pmem[3][PERC_MEMS];     /* each as left (PRESETS on EDIT; the stab's: smem) */
    /* (appended: LOAF BREAKER's LIMIT; an older save's: off) */
    uint8_t comp;                        /* SEL + K3: COMP, the whole mix compressed, 0 off .. 100 (its threshold 0 ..
                                          * -36 dB; was LIMIT's drive) */
    uint8_t rsv6[3];
    /* (appended: LOAF BREAKER's KICK EQ; an older save's: flat) */
    float keq[8];                        /* HOME's third part: LOW, MID, HIGH, FREQ, LO FREQ, MID Q, HI FREQ, LEVEL */
    /* (appended: LOAF BREAKER's RAVE STAB; an older save's: its first preset) */
    union {                              /* (LOAF BREAKER, 11 Nov 2026: the STAB's sound, unused since 0.209, holds the
                                          * black keys' layout: bk[0] BK_TAG, bk[1] the preset BKP_*, bk[2..12] the
                                          * CUSTOM layout, a function BKF_* each key; an older save's: CHOPPAH) */
        perc_voice_t pstab_was;
        uint8_t bk[24];
    } bkeys;
    uint8_t sslot;                       /* its memory playing, 0..9 */
    uint8_t rsv8[3];
    perc_voice_t smem[PERC_MEMS];        /* its memories */
    /* (appended: LOAF BREAKER's DROP and TRACK, ENV's parts; an older save's: a 4-bar build into the next pattern) */
    uint8_t drop_bars;                   /* the build: DROP_BARS's index (0: none, the drop at the bar's end) */
    uint8_t drop_after;                  /* the pattern the drop brings in: 0 the next one, 1.. that one */
    uint8_t drop_crash;                  /* the drop's crash, 0 off .. 100 */
    uint8_t trk;                         /* the track TRACKS edits and PLAY plays, 0..NTRACKS-1 */
    /* (appended: LOAF BREAKER's KICK FX, HOME's fourth part; an older save's: both empty) */
    brk_fxslot_t kfx[BRK_KFX_SLOTS];     /* the kick's own effects (FX's library but REVERB and DELAY) */
    brk_fxx_t kfxx[BRK_KFX_SLOTS];       /* their extras (MORE, MOD) */
    brk_mbc_t kmbc[BRK_KFX_SLOTS];       /* an MB COMP's detail */
    /* (appended: LOAF BREAKER's KNOB part of ENV, the SEL + knob functions' settings; an older save's: as they were) */
    uint8_t duck_att, duck_rel;          /* DUCK: its attack 0.1 .. 30 ms, its release 20 .. 800 ms (0..100) */
    uint8_t duck_what;                   /* 0 the break, 1 the break and the drums (not the kick) */
    uint8_t comp_ratio;                  /* COMP: COMP_RATIO's index */
    uint8_t comp_att, comp_rel;          /* its attack 0.1 .. 30 ms, its release 20 .. 800 ms (0..100) */
    uint8_t max_out;                     /* LOAF BREAKER: GLOBAL MORE's MAX OUT, the output's most at MASTER full: 0
                                          * the default (-3 dB), else 1 + its dB under full scale (0..12) */
    uint8_t selk_asg;                    /* SEL + K2's function (bits 0-3), SEL + K4's (4-7): 0 their own (TERROR 2,
                                          * TERROR), else ENV KNOB's choice (track.c's SK_CODE) */
    /* (appended: LOAF BREAKER's SEL + K1 and K3 assignable too; an older save's: DUCK, COMP) */
    uint8_t selk_asg2;                   /* SEL + K1's (bits 0-3), SEL + K3's (4-7), as selk_asg */
    /* (appended: LOAF BREAKER's BUILD part of ENV, drops unlike each other; an older save's: as they were) */
    uint8_t drop_style;                  /* the build: DS_* (0 the snare roll) */
    uint8_t drop_sweep;                  /* its filter: 0 HIGH-PASS, 1 LOW-PASS, 2 none, 3 either, by chance */
    uint8_t drop_riser;                  /* its rising noise, 0 none .. 100 (50: as it always was) */
    uint8_t drop_gap;                    /* silence before the drop: DROP_GAP's index (0 none) */
    /* (appended: LOAF BREAKER's kick SYNTH, EDIT KICK's SYNTH and SYNTH +; an older save's: as it always sounded) */
    int8_t ksyn[KS_N];                   /* the kick playing's (kick.h KS_*: -50 .. +50, TUNE -12 .. +12) */
    int8_t ksmem[KMEM_A][KS_N];          /* each kick memory's, as set.kmem */
    uint8_t kbass[4];                    /* LOAF BREAKER TEST: the kick's BASS part: GLIDE, REV BASS, its START, its
                                          * CURVE (0..100 each) */
    uint8_t drop_accel;                  /* (TEST) BUILD's tempo rise to the drop: 0 .. 30 % */
    uint8_t crt;                         /* GLO MORE K3 EFFECTS: the screen's effect, SFX_* (0 off, 1 CRT: an older
                                          * save's ON) */
    uint8_t scrfx;                       /* (appended: GLO MORE K1, the effect's AMOUNT, 0..100; an older save: 50) */
    uint8_t wzoom;                       /* (appended: GLO MORE PRESETS, WAVE ZOOM: CDJ_Z's index, 3 one bar) */
    uint8_t sfx_kick, sfx_drop, sfx_len; /* (appended: GLO FX SET K2-K4, the screen effects on a kick, at the drop, how
                                          * long; 0..100, 50 as it was) */
    uint8_t sfx_on;                      /* (appended: GLO EFFECTS, the screen effects on, several at once: bit e - 1 for
                                          * SFX_* e; none to start. crt, the one MORE picked before, is left) */
    uint8_t csub;                        /* (appended: the kick's CLEAN SUB, 0..100; an older save: 0, off) */
    uint8_t kmorph, kmorph_to;           /* (appended: KICK MORPH, 0..100 towards kick memory kmorph_to (0..9); 0: off) */
    uint8_t novice;                      /* (appended, LOAF BREAKER TEST: GLO MORE K1 MODE: 1 BEGINNER, the KICK menu's
                                          * advanced parts passed over (kept as they are); 0 EXPERT. A new FM-1 or RESET
                                          * ALL: BEGINNER; an older save: EXPERT, as it was) */
    uint8_t drop_kick;                   /* (appended) ENV DROP's PRESETS: the kick a SEL + ENV drop's build plays, 0 the
                                          * one playing, n KICK n (a track's DROP item: its own, TRACKS K1); back at the drop */
    uint8_t selk_hi;                     /* (appended) each SEL + knob's function code's fifth bit (bit 0 K1 .. bit 3 K4:
                                          * selk_asg2, selk_asg hold the four below; codes past 15); bit 4 the kick's
                                          * TERROR off (TER) */
    uint8_t pgrp;                        /* (appended) LOAF BREAKER: the patterns' group in RAM, 0..3 (P1-16 .. P49-64);
                                          * pattern its slot there */
    uint8_t fill_cfg;                    /* (appended) LOAF BREAKER: FILL (ARP; SEL + ARP its page): bit 0 LATCH, bits
                                          * 1-2 the drum it rolls, 3-4 how long, 5 in 8ths; 0: the snare, 16ths, the
                                          * bar's last beat, while held (as before) */
    uint8_t mel_vol;                     /* (appended) LOAF BREAKER: the MELODY's fader, 0.200-0.208 (unused since) */
    uint8_t brkv;                        /* (appended) the MELODY's mel_ver until 0.208; LOAF BREAKER (0.234, FLEXOR):
                                          * EXPERT's SEQ grid, the break's rows shown, BRKV_TAG | 0 HIDDEN, 1 the first 16
                                          * steps, 2 all 32 (it always plays 32): brk_view(); else (an older save) 32.
                                          * SEQ's BREAK, K4 (the settings at their 3840 bytes: no byte appended) */
    uint8_t wstyle;                      /* (appended) LOAF BREAKER: the waves' colours (the KICK menu's picture, HOME's
                                          * CDJ view): 0 REKORDBOX (the bands mixed: RGB, white where all are), 1 SERATO
                                          * (the bands as one hue, red to blue, never white); GLO EFFECTS K3 */
    uint8_t kfocus;                      /* (appended) LOAF BREAKER: the KICK menu's picture: 0 the WHOLE kick, 1 the PART
                                          * edited (a layer alone, the attack close, the tail whole); GLO EFFECTS K4 */
    uint8_t tut;                         /* (appended) LOAF BREAKER: the TUTORIAL (tutor.c): 0 off, 1.. its step running,
                                          * 0xFE finished; a new FM-1 starts it, the simulator not */
    uint8_t ezoom;                       /* (appended) LOAF BREAKER: EDIT's pictures (the kick, the drums), the time they
                                          * show: 0 AUTO (as each part wants), 1.. EZ_MS[]; SYSTEM › EFFECTS, PRESETS */
    uint8_t pxv;                         /* (appended) LOAF BREAKER: PX_OK: smem (the MELODY's memories, unused since
                                          * 0.209) holds the drums' extras, perc_x(); else cleared at the load */
} settings_t;
#define PX_OK 0xA5u
#define BRKV_TAG 0xB0u
static uint32_t brk_view(void);          /* (below: set is declared after) */
/* (LOAF BREAKER, 0.244: FILL gone, set.fill_cfg unused) */
enum { DS_SNARE, DS_KICK, DS_BOTH, DS_STUTTER, DS_SWEEP, DS_RANDOM, DS_N };
static const uint8_t DROP_GAP[5] = {0u, 1u, 2u, 4u, 8u};   /* its steps: none, a 16th, an 8th, a beat, half a bar */
static const float COMP_RATIO[8] = {1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 20.0f};
static float att_ms(uint32_t v) { return 0.1f * brk_exp2f_c((float)(v > 100u ? 100u : v) * 0.0825f); }
static float rel_ms(uint32_t v) { return 20.0f * brk_exp2f_c((float)(v > 100u ? 100u : v) * 0.0532f); }
#define SET_SIZE_MIN offsetof(settings_t, rsv2)   /* the shortest save that loads: CHP7's first size */
_Static_assert(sizeof(settings_t) <= 3840u, "a store object holds 3840 bytes");
static settings_t set;
/* LOAF BREAKER (11 Nov 2026, FLEXOR): the black keys' functions, BKF_*: the eleven pads (brk.h PAD_*: FREEZE and DUB
 * back), then ARP (an OVERLAY effect), KIL (the sequencer's kicks quiet), DST (the kick's DISTO), SUB (its SUB DRIVE).
 * A layout: the function of each key, left to right. Presets: CHOPPAH (CHOPPAH's keys; BEGINNER's only), FLEXOR LAYOUT
 * (LOAF BREAKER's until 0.244), CUSTOM (each key its own: ARP › BLACK KEYS) */
enum { BKF_ARP = BRK_NPADS, BKF_KIL, BKF_DST, BKF_SUB, BKF_N };
enum { BKP_CHOPPAH, BKP_FLEXOR, BKP_CUSTOM, BKP_N };
#define BK_TAG 0xB4u
#define BK_VIEW 14u                      /* (bk[14]: HOME's VIEW, 0 the waveform, 1 the slices, 0.9.2) */
#define BK_BHALF 13u                     /* (bk[13]: BREAK TIME, 0 NORMAL, 1 HALF: the break at half the tempo, 0.256) */
static const uint8_t BK_LAY[2][11] = {{PAD_ROLL8, PAD_ROLL16, PAD_ROLL32, PAD_REVERSE, PAD_FREEZE, PAD_HALF,
                                       PAD_TAPESTOP, PAD_GATE, PAD_RANDOM, PAD_FILTER, PAD_DUB},
                                      {PAD_ROLL8, PAD_ROLL16, PAD_ROLL32, PAD_REVERSE, PAD_RANDOM, PAD_HALF,
                                       PAD_TAPESTOP, BKF_ARP, BKF_KIL, BKF_DST, BKF_SUB}};
static uint32_t bk_preset(void)          /* the preset in use (BEGINNER: CHOPPAH, always) */
{
    if (set.novice || set.bkeys.bk[0] != BK_TAG || set.bkeys.bk[1] >= BKP_N)
        return BKP_CHOPPAH;
    return set.bkeys.bk[1];
}
static uint32_t bk_fn(uint32_t j)        /* black key j's (0..10) function */
{
    uint32_t p = bk_preset(), f;
    j %= 11u;
    if (p != BKP_CUSTOM)
        return BK_LAY[p][j];
    f = set.bkeys.bk[2u + j];
    return f < BKF_N ? f : BK_LAY[0][j];
}
static void bk_defaults(void)
{
    uint32_t j;
    set.bkeys.bk[0] = BK_TAG;
#if defined(PLAT_HOST) && !defined(PLAT_WEB)
    set.bkeys.bk[1] = BKP_FLEXOR;        /* (TEST: the simulator's scenarios, EXPERT, as written for it) */
#else
    set.bkeys.bk[1] = BKP_CHOPPAH;       /* (a new FM-1, RESET ALL: CHOPPAH) */
#endif
    for (j = 0; j < 11u; j++)
        set.bkeys.bk[2u + j] = BK_LAY[0][j];   /* (CUSTOM begins as CHOPPAH) */
    set.bkeys.bk[BK_BHALF] = 0u;
}
static uint32_t brk_view(void) { return (set.brkv & 0xF0u) == BRKV_TAG && (set.brkv & 15u) <= 2u ? set.brkv & 15u : 2u; }
static uint32_t max_out_db(void) { return set.max_out ? (set.max_out - 1u > 12u ? 12u : set.max_out - 1u) : 3u; }
static perc_voice_t *perc_voice(uint32_t v)   /* LOAF BREAKER: a drum's sound, PV_* (the snare and the hat in perc,
                                          * the rest in pmore) */
{
    v %= PERC_VOICES;
    static perc_voice_t none;            /* (PV_STAB: an empty place, silent; its bytes the black keys') */
    return v < 2u ? &set.perc.v[v] : v == PV_CLAP ? &set.pmore[0] : &none;
}
/* LOAF BREAKER (11 Nov 2026): a drum's extras (PX_*: its drop, its saturation, LIFT, ROOM, BEGINNER's FX, its preset),
 * the memory playing's, in the MELODY's old place: smem, 240 bytes, three drums of ten memories of PX_N */
_Static_assert(sizeof(((settings_t *)0)->smem) == 3u * PERC_MEMS * PX_N, "smem holds the drums' extras");
static uint8_t *perc_xm(uint32_t v, uint32_t i) { return (uint8_t *)(void *)set.smem + ((v % 3u) * PERC_MEMS + i % PERC_MEMS) * PX_N; }
static uint8_t *perc_x(uint32_t v) { return perc_xm(v, set.pslot[v % 3u]); }
static uint8_t *perc_slot(uint32_t v)    /* its memory playing */
{
    v %= PERC_VOICES;
    return v == PV_STAB ? &set.sslot : &set.pslot[v];
}
static perc_voice_t *perc_mem(uint32_t v, uint32_t i)   /* its memory i */
{
    v %= PERC_VOICES;
    i %= PERC_MEMS;
    return v == PV_STAB ? &set.smem[i] : &set.pmem[v][i];
}
static uint8_t set_dirty;
static uint8_t foot_dirty;               /* a setting changed: the slicer's foot shows the new value */

/* LOAF BREAKER: the KICK LAB (EDIT KICK's LAB section: TAIL, TAIL +, STACK, TOOLS), the kick playing's and each kick
 * memory's, as set.kick and set.kmem; their own store object (OBJ_KLAB). All 0: as the kicks always sounded */
#define KLAB_MAGIC 0x31424C4Bu           /* "KLB1" */
typedef struct {
    uint32_t magic;
    kick_lab_t cur;                      /* the kick playing's */
    kick_lab_t mem[KMEM_A];              /* each kick memory's (KICK 1..10; 11..20: km2's) */
} klab_obj_t;
static klab_obj_t klab;
static kick_t *kmem_at(uint32_t n);     /* (the kick memory n, 0..19, wherever it is kept: below) */
static int8_t *ksmem_at(uint32_t n);
static kick_lab_t *klmem_at(uint32_t n);
static void km2_lab_clear(void);
static uint8_t klab_solo;                /* STACK's SOLO (not saved): 0 both, 1 the attack, 2 the tail */
static void klab_defaults(void)
{
    memset(&klab, 0, sizeof klab);
    km2_lab_clear();                     /* (KICK 11..20's too) */
    klab.magic = KLAB_MAGIC;
}
static void klab_send(void)              /* the LAB to the engine, STACK's attack with it (its kick memory's) */
{
    uint32_t n = klab.cur.src ? (uint32_t)(klab.cur.src - 1u) % KICK_MEMS : 0u;
    kick_set_lab(&klab.cur);
    if (!klab.cur.src)
        kick_set_stack(0, 0, 0);
    else if (n == set.kslot % KICK_MEMS)   /* (its own: the kick playing, as it is) */
        kick_set_stack(&set.kick, set.ksyn, &klab.cur);
    else
        kick_set_stack(kmem_at(n), ksmem_at(n), klmem_at(n));
    kick_set_solo(klab_solo);
}
/* LOAF BREAKER: the kick to the engine: the one playing (set.kick, its SYNTH set.ksyn), or with KICK MORPH up that
 * far towards kick memory kmorph_to: every level, pitch, decay, tone, effect setting, TERROR and SYNTH setting in
 * between; the layers' effects and TERROR + 's TYPE the nearer one's. CLEAN SUB with it */
static int32_t morph_i(int32_t a, int32_t b, uint32_t m) { return a + (b - a) * (int32_t)m / 100; }
/* LOAF BREAKER: TER (the tenth black key), as SUB's DRIVE: the kick's TERROR off, whatever its amount (kept as it is),
 * or on again; the kick's own (selk_hi bit 4, with its memory: kx_put), saved */
#define terror_off (!set.novice && (set.selk_hi & 16u))   /* (LOAF BREAKER, 0.229: BEGINNER has no DST key: its
                                          * DISTO knob always heard) */
static void kick_send(void)
{
    uint32_t m = set.kmorph > 100u ? 100u : set.kmorph, i, j;
    if (!m) {
        kick_t k = set.kick;
        if (terror_off)
            KICK_TERROR(&k) = 0u;
        kick_set(&k);
        kick_set_synth(set.ksyn);
    } else {
        kick_t k = set.kick;
        const kick_t *b = kmem_at(set.kmorph_to % KICK_MEMS);
        int8_t sy[KS_N];
        float t = (float)m * 0.01f;
        for (i = 0; i < KICK_LAYERS; i++) {
            kick_layer_t *l = &k.l[i];
            const kick_layer_t *o = &b->l[i];
            l->level += (o->level - l->level) * t;
            l->pitch += (o->pitch - l->pitch) * t;
            l->decay += (o->decay - l->decay) * t;
            l->tone += (o->tone - l->tone) * t;
            if (l->fx == o->fx) {
                l->fa += (o->fa - l->fa) * t;
                l->fb += (o->fb - l->fb) * t;
                l->wet += (o->wet - l->wet) * t;
            } else if (m >= 50u) {
                l->fx = o->fx;
                l->fa = o->fa;
                l->fb = o->fb;
                l->wet = o->wet;
            }
            for (j = 0; j < 3u; j++) {   /* (the TERRORs' bytes: the amounts 0..100, TYPE the nearer, the rest int8) */
                int unsign = j == 0u && i != KL_GRIT, type = i == KL_GRIT && j == 2u;
                if (type)
                    l->rsv[j] = m >= 50u ? o->rsv[j] : l->rsv[j];
                else if (unsign)
                    l->rsv[j] = (uint8_t)morph_i(l->rsv[j], o->rsv[j], m);
                else
                    l->rsv[j] = (uint8_t)(int8_t)morph_i((int8_t)l->rsv[j], (int8_t)o->rsv[j], m);
            }
        }
        k.intensity += (b->intensity - k.intensity) * t;
        for (j = 0; j < KS_N; j++)
            sy[j] = (int8_t)morph_i(set.ksyn[j], ksmem_at(set.kmorph_to % KICK_MEMS)[j], m);
        if (terror_off)
            KICK_TERROR(&k) = 0u;
        kick_set(&k);
        kick_set_synth(sy);
    }
    kick_set_clean_sub((float)(set.csub > 100u ? 100u : set.csub) * 0.01f);
    klab_send();
}
static void settings_defaults(void)
{
    uint32_t k;
    uint8_t *z = (uint8_t *)&set;
    for (k = 0; k < sizeof set; k++)     /* (every field: no leftovers from a save that didn't load) */
        z[k] = 0u;
    set.magic = SET_MAGIC;
    set.kbass[2] = set.kbass[3] = 50u;
    set.scrfx = 50u;                     /* (the screen effect's AMOUNT: half) */
    set.sfx_on = 0u;                     /* (the screen effects: none at the start and after RESET ALL; the site's
                                          * screenshots clean) */
    set.wzoom = 3u;                      /* (the scrolling wave: a bar across, as it was) */
    bk_defaults();                       /* (LOAF BREAKER: the black keys: CHOPPAH) */
    set.skin = 0x8Au;                    /* (LOAF BREAKER, 0.240: RED, common.c's SKIN_NEW | SKIN_DEFAULT) */
#if defined(PLAT_HOST) && !defined(PLAT_WEB)
    set.novice = 0u;                     /* (TEST: the simulator's scenarios and the manual's screens see it all: EXPERT;
                                          * a scenario turns GLO MORE's K1 for BEGINNER) */
    if (getenv("LOAF_ASK"))              /* (LOAF BREAKER, 0.229: the first start's question, for its scenario) */
        set.tut = 0xFDu;
#else
    set.novice = 1u;                     /* (TEST: BEGINNER to start, on the FM-1 and the web emulator) */
    set.tut = 0xFDu;                     /* (LOAF BREAKER, 0.229: the mode asked first, tutor.c TUT_ASK; 0.211-0.228 the
                                          * TUTORIAL straight away) */
#endif
    set.sfx_kick = set.sfx_drop = set.sfx_len = 50u;   /* (TEST: REV BASS from 3/8 of the beat, a curve between) */
    set.tempo_x10 = 1700u;               /* jungle tempo */
    set.swing = 50u;
    set.gen.anchor = 0.7f;
    set.gen.roll = 0.3f;
    set.gen.fill = 0.5f;
    set.gen.phrase_bars = 4u;
    brk_fx_defaults(&set.fx);
    set.pitch_keep = 1u;
    set.grain_ms = 30u;
    set.gate = 100u;
    set.snap = 5u;                       /* 1 slice */
    set.brk_level = 100u;
    set.drums_chain = 0u;                /* (0.9.2, FLEXOR: AROUND the FX chain by default, any mode) */
    brk_padset_defaults(&set.pads);
    for (k = 0; k < BRK_KEYS; k++) {
        brk_keyset_defaults(&set.keys[k]);
        brk_slicefx_defaults(&set.slx[k]);
    }
    kick_defaults(&set.kick);
    for (k = 0; k < KICK_MEMS; k++) {    /* LOAF BREAKER: the kick memories begin as the built-in kicks (7..10: the
                                          * first four again) */
        kick_preset(kmem_at(k), k % KICK_PRESETS);
        (*kmem_at(k)).volume = set.kick.volume;
    }
    set.kslot = 3u;                      /* (ROTTERDAM's, set.kick's) */
    for (k = 0; k < 8u; k++)
        set.keq[k] = 0.5f;               /* (LOAF BREAKER: the KICK EQ flat) */
    {                                    /* (the snare and the hat where saves have them, the rest in pmore) */
        perc_t d;
        perc_defaults(&d);
        set.perc.v[0] = d.v[0];
        set.perc.v[1] = d.v[1];
        set.perc.volume = d.volume;
        set.pmore[0] = d.v[PV_CLAP];
        for (k = 0; k < PERC_VOICES * PERC_MEMS; k++)   /* (the memories begin as the presets, again and again) */
            perc_preset(perc_mem(k / PERC_MEMS, k % PERC_MEMS), k / PERC_MEMS, k % PERC_MEMS % PERC_PRESETS);
    }
    memset(set.smem, 0, sizeof set.smem);   /* (LOAF BREAKER: the drums' extras: none, as they always sounded) */
    set.pxv = PX_OK;
    set.brkv = BRKV_TAG | 2u;            /* (LOAF BREAKER: the break's 32 steps on SEQ's grid) */
    for (k = 0; k < 16u; k++)            /* the overlays: full, the sweeps and the dirt a little less */
        set.ovl_amt[k] = k == OV_DIVE || k == OV_RISE || k == OV_CRUSH || k == OV_DIST || k == OV_BOOM ? 70u : 100u;
    set.ovl_rate = 2u;                   /* 1/16 */
    set.ovl_sweep = 2u;                  /* a bar */
    for (k = 0; k < BRK_FX_SLOTS; k++) {
        brk_mbc_defaults(&set.mbc[k]);
        brk_fxx_defaults(&set.fxx[k], set.fx.slot[k].type);
    }
    set.duck_att = 19u;                  /* (LOAF BREAKER: DUCK ~0.3 ms in, ~75 ms back; COMP 4:1, ~3 ms, ~150 ms) */
    set.duck_rel = 36u;
    set.comp_ratio = 3u;
    set.comp_att = 59u;
    set.comp_rel = 55u;
    set.drop_bars = 3u;                  /* (LOAF BREAKER: DROP: a 4-bar build, into the next pattern, the crash full) */
    set.drop_crash = 100u;
    set.drop_riser = 50u;
    for (k = 0; k < BRK_KFX_SLOTS; k++) {   /* (KICK FX: empty) */
        brk_fx_slot_defaults(&set.kfx[k], FXT_EMPTY);
        set.kfx[k].on = 0u;
        brk_fxx_defaults(&set.kfxx[k], FXT_EMPTY);
        brk_mbc_defaults(&set.kmbc[k]);
    }
}

static int pitch_keeps(void) { return set.pitch_keep != 2u; }   /* KEEP LENGTH unless SPEED was chosen */

/* the snap: where a white key pressed while playing lands, in slices (GLOBAL, second half, K3) */
#define SNAPS 7u
static const float SNAP_SL[SNAPS] = {0.0f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
static const char *const SNAP_NAME[SNAPS] = {"OFF", "1/8 SLICE", "1/4 SLICE", "1/2 SLICE", "1 SLICE", "2 SLICES",
                                             "4 SLICES"};
static const char *const SNAP_SHORT[SNAPS] = {"OFF", "1/8 SL", "1/4 SL", "1/2 SL", "1 SL", "2 SL", "4 SL"};
static uint32_t snap_index(void) { return set.snap && set.snap <= SNAPS ? set.snap - 1u : 4u; }

static void skin_apply(uint32_t s);    /* (ui/common.c) */
static uint32_t skin_old(uint32_t s); /* (ui/common.c: an older save's skin, 0..17, to the 1.0's) */
/* LOAF BREAKER: the FX chain's part of settings_apply (its four slots and the kick's two, their extras), and the
 * kick's (its layers, SYNTH, MORPH, CLEAN SUB, BASS, EQ, the ARP's REESE in its key): a knob of theirs applies them
 * alone (changed_fx, changed_kick) */
static void fx_apply(void)
{
    uint32_t k;
    if (set.novice) {                    /* (LOAF BREAKER, 0.229: BEGINNER's chain two slots, the others muted) */
        brk_fx_t f = set.fx;
        for (k = 2u; k < BRK_FX_SLOTS; k++)
            f.slot[k].on = 0u;
        brk_fx_set(&f);
    } else
        brk_fx_set(&set.fx);
    for (k = 0; k < BRK_FX_SLOTS; k++) {
        brk_fx_set_mbc(k, &set.mbc[k]);
        brk_fx_set_fxx(k, &set.fxx[k]);
    }
    for (k = 0; k < BRK_KFX_SLOTS; k++) {   /* (LOAF BREAKER: KICK FX: slots 4, 5) */
        brk_fx_set_kick_slot(k, &set.kfx[k]);
        brk_fx_set_mbc(BRK_FX_SLOTS + k, &set.kmbc[k]);
        brk_fx_set_fxx(BRK_FX_SLOTS + k, &set.kfxx[k]);
    }
    brk_fx_tempo((float)set.tempo_x10 * 0.1f);
}
static void kick_apply(void)
{
    ovl_reese_hz(kick_pitch_hz(KL_SUB, set.kick.l[KL_SUB].pitch));   /* (the ARP's REESE in the kick's key) */
    brk_fx_set_kick_eq(set.keq);
    kick_send();                         /* (LOAF BREAKER: the kick, its SYNTH, MORPH and CLEAN SUB) */
    kick_set_bass((float)set.kbass[0] * (float)set.kbass[0] * 0.04f, (float)set.kbass[1] * 0.01f,
                  (float)set.kbass[2] * 0.01f, (float)set.kbass[3] * 0.01f);   /* (TEST: its BASS) */
}
static void perc_apply(void)             /* all the drums' voices for the engine (LOAF BREAKER: the MELODY's too) */
{
    perc_t pa;
    uint32_t k;
    for (k = 0; k < PERC_VOICES; k++)
        pa.v[k] = *perc_voice(k);
    pa.volume = set.perc.volume;
    perc_set(&pa);
    for (k = 0; k < 3u; k++)             /* (LOAF BREAKER: their extras) */
        perc_set_x(k, perc_x(k));
}
static void settings_apply(void)
{
    brk_tempo((float)set.tempo_x10 * 0.1f);
    brk_half_time(set.bkeys.bk[BK_BHALF] & 1u);   /* (LOAF BREAKER, 0.256: SYSTEM › MIDI K4 BREAK TIME) */
    brk_pitch((float)set.pitch);
    brk_swing((float)set.swing * 0.01f);
    brk_gen(&set.gen);
    fx_apply();
    set.pads.roll_pitch = 0.0f;          /* LOAF BREAKER: the rolls' pitch gone (an older save's: off) */
    brk_padset(&set.pads);
    brk_pitch_mode(pitch_keeps(), (float)(set.grain_ms ? set.grain_ms : 30u));
    brk_half_master(!(set.half_mt_off & 1u));
    brk_vinyl((set.vinyl & 3u) != 2u);
    brk_gate((float)(set.gate ? set.gate : 100u) * 0.01f);
    brk_snap(SNAP_SL[snap_index()]);
    kick_apply();
    perc_apply();
    skin_apply(set.skin & 0x7Fu);
    {
        uint32_t k;
        for (k = 0; k < BRK_KEYS; k++) {
            brk_keyset(k, &set.keys[k]);
            brk_slicefx(k, &set.slx[k]);
        }
    }
}

static uint8_t apply_due, apply_kick, apply_fx;   /* LOAF BREAKER: a change waiting for settings_apply, or for the
                                          * kick's or the FX chain's part alone (apply_flush) */
static void changed(void)
{
    set_dirty = 1u;
    foot_dirty = 1u;
    apply_due = 1u;                      /* (TEST: applied once a pass, after the input: a knob turned fast, its many
                                          * detents one settings_apply, not one each) */
}
static void changed_kick(void)          /* (a change to the kick alone: its part applied) */
{
    set_dirty = 1u;
    foot_dirty = 1u;
    apply_kick = 1u;
}
static void changed_fx(void)            /* (a change to the FX chain alone: its part applied) */
{
    set_dirty = 1u;
    foot_dirty = 1u;
    apply_fx = 1u;
}
static void apply_flush(void)
{
    if (apply_due)
        settings_apply();
    else {
        if (apply_kick)
            kick_apply();
        if (apply_fx)
            fx_apply();
    }
    apply_due = apply_kick = apply_fx = 0u;
}

/* ------------------------------------------------------------------ patterns --- */
/* the pattern sequencer (PRD B-20..B-22): 16 patterns, each 32 steps of the break (2 bars: slice or follow, pitch,
 * ratchet, reverse; the engine plays them, brk_step_set) and 16-step lanes for the kick, the snare and the hat (each
 * bar). Saved with SAVE in their own store object. PRESETS picks one: while playing it takes over at the end of the
 * loop */
#define NPAT 16u
#define PAT_STEPS 32u
#define PAT_MAGIC 0x33544150u            /* "PAT3" (GLOUPI BREAKS: the kick over four bars) */
#define PAT2_MAGIC 0x32544150u           /* "PAT2": the kick two bars; loaded with them twice */
#define PAT1_MAGIC 0x31544150u           /* "PAT1": the kick one bar; loaded with it four times */
enum { LANE_KICK, LANE_SNARE, LANE_HAT, NLANES };
/* LOAF BREAKER: the drums the pattern plays (LANE_* the first three, pat_t's lane[]), DR_SNARE.. the PV_* voices + 1 */
enum { DR_KICK, DR_SNARE, DR_HAT, DR_CLAP, NDRUMS };   /* (LOAF BREAKER, 0.209: the MELODY's out: kicks and drums only) */
#define NDRUMS_SAVED 5u                  /* (pats.hits' layout in the saves: the MELODY's place kept) */
typedef struct {
    uint8_t st[PAT_STEPS][4];            /* per step: slice + 2 (0 off, 1 follow the loop, 2.. slice 1..), pitch
                                          * (int8), flags (bit 0 reverse, bits 1-2 ratchet - 1), spare (GLOUPI
                                          * BREAKS: steps 0-3's the kick's bars 3 and 4, kick_bar; 4-5's the
                                          * snare's bar 2, snare_bar; 6's the lanes' lengths, PAT_LEN) */
    uint16_t lane[NLANES];               /* bit n: the drum hits on step n of each bar (the kick: of its first) */
    uint16_t kick2;                      /* GLOUPI BREAKS: the kick's second bar (steps 17-32); was PAT1's rsv */
} pat_t;                                 /* 136 bytes */
/* LOAF BREAKER: the TRACKS (ENV): each a list of patterns, each played its loops (a loop: the pattern's length), and
 * DROPs between them (the build, then the drop into the pattern after it), to its END; then from its start again,
 * a stop, or the last pattern held. Saved with the patterns (after them: an older save, its patterns alone, loads) */
#define NTRACKS 8u
#define TRK_ITEMS 32u
#define TI_END 0u                        /* it[]: 0 the end, 1..NPAT that pattern, TI_DROP a drop */
#define TI_DROP 0xFEu
#define TI_GAP 0xFDu                     /* LOAF BREAKER: a gap (ENV held on TRACKS): to be filled, played past */
typedef struct {
    uint8_t it[TRK_ITEMS];
    uint8_t rep[TRK_ITEMS];              /* a pattern's loops, 1..16 */
    uint8_t end;                         /* at its end: 0 LOOP, 1 STOP, 2 HOLD */
    uint8_t rsv[3];
} track_t;
/* LOAF BREAKER: each effect's settings as last left, kept when a slot turns to another effect and back (FX's
 * PRESETS; EDIT KICK's FX, each layer's); saved after the tracks. 0..65535 for 0..1 */
#define FXM_N (4u + BRK_FXX)             /* an effect's P1-P3, MIX, then its extras (MORE, MOD) */
typedef struct {
    uint16_t fx[BRK_NFXT][FXM_N];
    uint16_t kfx[KICK_LAYERS][KICK_FX - 1u][3];   /* a kick layer's effect (KFX_DRIVE..): its two settings, its mix */
    uint16_t fx_ok;                      /* bit t: fx[t] holds them (none: the effect's starting settings) */
    uint16_t kfx_ok;                     /* bit layer * 4 + effect - 1 */
} fxmem_t;
typedef struct {
    uint32_t magic;
    pat_t p[NPAT];
    track_t trk[NTRACKS];
    fxmem_t mem;
    uint16_t kfm[BRK_NFXT][FXM_N];       /* KICK FX's effects as left (its own: not the chain's) */
    uint16_t kfm_ok, kfm_rsv;
    uint8_t kn[NPAT][13];                /* LOAF BREAKER: the kick's notes past its 38th step (kick_note) */
    uint8_t hits[NPAT][NDRUMS_SAVED];    /* LOAF BREAKER: each drum's HITS in the pattern (SEQ's K2): 0, 1 once a step
                                          * (as ever), 2..4 that many times in its step */
} pats_t;
static pats_t pats PLAT_POOL;            /* (LOAF BREAKER: in the pool, 3.7 KB of RAM freed) */
#define PATS_SIZE_OLD offsetof(pats_t, trk)   /* (a save before the tracks) */
#define PATS_SIZE_TRK offsetof(pats_t, mem)   /* (one before the effects' memory) */
#define PATS_SIZE_MEM offsetof(pats_t, kfm)   /* (one before KICK FX's) */
#define PATS_SIZE_KFM offsetof(pats_t, kn)    /* (one before the kick's notes on every step) */
#define PATS_SIZE_KN offsetof(pats_t, hits)   /* (one before the drums' HITS) */
/* LOAF BREAKER: each drum step's own settings (SEQ: a step held, K2-K4), a byte each: bits 0-1 HITS - 1 (it taps 1..4
 * times in its step), bits 2-4 CHANCE (0: always .. 7: one time in eight), bits 5-7 NUDGE (0 on the step .. 7 that
 * many 16ths of it late). The kick's 64 steps, then the others' 32 each; their own store object (OBJ_STEP). An older
 * save: none, and the pattern's HITS a drum (pats.hits) on each of its steps */
#define STEPB_N 192u
#define STEPS_MAGIC 0x31505453u           /* "STP1" */
typedef struct {
    uint32_t magic;
    uint8_t b[NPAT][STEPB_N];
    uint8_t tb[8][32];                   /* LOAF BREAKER: each track item's break (TRACKS K4): 0 the one playing, 1..
                                          * that break + 1 (NTRACKS x TRK_ITEMS; an older save: none) */
    uint8_t kx[KMEM_A][KX_N];            /* (appended) each kick memory's knobs, kx_put's order: ENV KNOB's choices,
                                          * CLEAN SUB, BASS (GLIDE, REV BASS, START, CURVE), MORPH and its target; the
                                          * kick playing's in set (selk_asg ..), here when another is picked */
    uint8_t tk[8][32];                   /* (appended) each track item's kick: 0 the one playing, 1.. KICK n (TRACKS K1;
                                          * NTRACKS x TRK_ITEMS; an older save: none) */
    uint8_t kxh[KMEM_A];                 /* (appended) each kick memory's selk_hi (kx's, as kx_put) */
    uint8_t kpre[4][9];                  /* (appended) LOAF BREAKER: STYLE's PRESET 1-4, the same for every kick: the
                                          * four knobs' codes (sk_code's), their values (0..200, sk_norm's), 1 stored */
} steps_t;
#define STEPS_SIZE_OLD offsetof(steps_t, tb)
#define STEPS_SIZE_TB offsetof(steps_t, kx)   /* (one before the kicks' own knobs) */
#define STEPS_SIZE_KX offsetof(steps_t, tk)   /* (one before the track items' kicks) */
#define STEPS_SIZE_TK offsetof(steps_t, kxh)  /* (one before the knobs' fifth bits) */
#define STEPS_SIZE_KXH offsetof(steps_t, kpre)   /* (one before STYLE's presets) */
_Static_assert(sizeof(steps_t) <= 3840u, "a store object holds 3840 bytes");

static steps_t steps PLAT_POOL;

/* LOAF BREAKER (0.9.2, FLEXOR): KICK 11..20, as KICK 1..10 are kept elsewhere: the kick, its SYNTH, its knobs (kx, kxh)
 * and its KICK LAB; their own store object (OBJ_KMEM2, the hoover notes' old room), saved with the rest. An older save
 * has none: they begin as the built-in kicks (settings_defaults' order), the knobs as the ones playing, no LAB */
#define KM2_MAGIC 0x324D4B4Bu            /* "KKM2" */
typedef struct {
    uint32_t magic;
    kick_t kmem[KICK_MEMS - KMEM_A];
    int8_t ksmem[KICK_MEMS - KMEM_A][KS_N];
    uint8_t kx[KICK_MEMS - KMEM_A][KX_N];
    uint8_t kxh[KICK_MEMS - KMEM_A];
    kick_lab_t lab[KICK_MEMS - KMEM_A];
} km2_t;
_Static_assert(sizeof(km2_t) <= 3840u, "a store object holds 3840 bytes");
static km2_t km2 PLAT_POOL;
static kick_t *kmem_at(uint32_t n)
{
    n %= KICK_MEMS;
    return n < KMEM_A ? &set.kmem[n] : &km2.kmem[n - KMEM_A];
}
static int8_t *ksmem_at(uint32_t n)
{
    n %= KICK_MEMS;
    return n < KMEM_A ? set.ksmem[n] : km2.ksmem[n - KMEM_A];
}
static uint8_t *kx_at(uint32_t n)
{
    n %= KICK_MEMS;
    return n < KMEM_A ? steps.kx[n] : km2.kx[n - KMEM_A];
}
static uint8_t *kxh_at(uint32_t n)
{
    n %= KICK_MEMS;
    return n < KMEM_A ? &steps.kxh[n] : &km2.kxh[n - KMEM_A];
}
static kick_lab_t *klmem_at(uint32_t n)
{
    n %= KICK_MEMS;
    return n < KMEM_A ? &klab.mem[n] : &km2.lab[n - KMEM_A];
}
static void km2_lab_clear(void)
{
    memset(km2.lab, 0, sizeof km2.lab);
}

/* LOAF BREAKER: 64 patterns, four groups of 16 (P1-16, P17-32, P33-48, P49-64). The group in RAM is pats.p, pats.kn,
 * pats.hits and steps.b (set.pattern its slot, set.pgrp the group); each group has its own three store objects
 * (OBJ_PG0, OBJ_KG0, OBJ_SG0 + g). A save writes the group in RAM to its own (OBJ_PAT and OBJ_STEP keep the tracks and
 * the rest, and the group in RAM: an older firmware reads that one as P1-16); another group comes in from its own (a
 * read: the sound plays on). Leaving a group whose patterns changed since: saved first while stopped, refused while
 * playing (a write stops the sound for a moment): SAVE or STOP first */
#define NGRP 4u
#define NPAT_ALL (NPAT * NGRP)
#define KH_SIZE (sizeof pats.kn + sizeof pats.hits)
_Static_assert(offsetof(pats_t, hits) == offsetof(pats_t, kn) + sizeof(((pats_t *)0)->kn), "kn, hits: one run");
static uint32_t grp_sum;                 /* the group's hash as it came in or was saved */
static uint8_t grp_new;                  /* the group has none of its own objects yet (an older save's P1-16) */
static uint32_t pat_cur(void) { return set.pgrp % NGRP * NPAT + set.pattern % NPAT; }   /* 0..63 */
static uint32_t grp_hash(void)
{
    const uint8_t *r[3] = {(const uint8_t *)pats.p, pats.kn[0], steps.b[0]};
    const uint32_t n[3] = {sizeof pats.p, KH_SIZE, sizeof steps.b};
    uint32_t h = 2166136261u, i, k;
    for (k = 0; k < 3u; k++)
        for (i = 0; i < n[k]; i++)
            h = (h ^ r[k][i]) * 16777619u;
    return h;
}
static int grp_changed(void) { return grp_new || grp_hash() != grp_sum; }
static uint8_t wiped;                    /* LOAF BREAKER (0.263): SYSTEM › RESET WIPE MEMORY done: nothing saved until a
                                          * restart (track.c wipe_step) */
static int grp_save(void)                /* the group in RAM to its objects: 1 done */
{
    uint32_t g = set.pgrp % NGRP;
    int ok;
    if (wiped)
        return 0;
    ok = plat_store_save(OBJ_PG0 + g, pats.p, sizeof pats.p) == 0 &&
             plat_store_save(OBJ_KG0 + g, pats.kn, KH_SIZE) == 0 &&
             plat_store_save(OBJ_SG0 + g, steps.b, sizeof steps.b) == 0;   /* (LOAF BREAKER, 0.209: OBJ_NG0 + g, the
                                          * MELODY's notes, no more) */
    if (ok) {
        grp_sum = grp_hash();
        grp_new = 0u;
    }
    return ok;
}
static int grp_has(uint32_t g)           /* the group's own objects there, whole */
{
    return plat_store_size(OBJ_PG0 + g) == (int)sizeof pats.p && plat_store_size(OBJ_KG0 + g) == (int)KH_SIZE &&
           plat_store_size(OBJ_SG0 + g) == (int)sizeof steps.b;
}
static void grp_blank(void)              /* the group in RAM: 16 empty patterns (pat_defaults' own) */
{
    uint32_t i, k;
    for (i = 0; i < NPAT; i++) {
        for (k = 0; k < PAT_STEPS; k++) {
            pats.p[i].st[k][0] = 1u;
            pats.p[i].st[k][1] = pats.p[i].st[k][2] = pats.p[i].st[k][3] = 0u;
        }
        pats.p[i].lane[LANE_KICK] = pats.p[i].lane[LANE_SNARE] = pats.p[i].lane[LANE_HAT] = 0u;
        pats.p[i].kick2 = 0u;
        pats.p[i].st[6][3] = (uint8_t)(set.novice ? 1u : 0u);   /* (LOAF BREAKER, 0.227: BEGINNER, the kick 32 steps) */
    }
    memset(pats.kn, 0, KH_SIZE);
    memset(steps.b, 0, sizeof steps.b);
}
static void grp_load(uint32_t g)         /* group g into RAM (none saved yet: empty) */
{
    g %= NGRP;
    if (!grp_has(g) || plat_store_load(OBJ_PG0 + g, pats.p, sizeof pats.p) != (int)sizeof pats.p ||
        plat_store_load(OBJ_KG0 + g, pats.kn, KH_SIZE) != (int)KH_SIZE ||
        plat_store_load(OBJ_SG0 + g, steps.b, sizeof steps.b) != (int)sizeof steps.b)
        grp_blank();
    set.pgrp = (uint8_t)g;
    grp_sum = grp_hash();
    grp_new = 0u;
}
static int store_all(void)               /* SAVE: the settings, the patterns (the group in RAM too), the steps: 1 done */
{
    if (wiped)                           /* (LOAF BREAKER, 0.263: WIPE MEMORY done: nothing saved until a restart) */
        return 0;
    return plat_store_save(OBJ_SETTINGS, &set, sizeof set) == 0 && plat_store_save(OBJ_PAT, &pats, sizeof pats) == 0 &&
           plat_store_save(OBJ_STEP, &steps, sizeof steps) == 0 && grp_save() &&
           plat_store_save(OBJ_KLAB, &klab, sizeof klab) == 0 &&
           plat_store_save(OBJ_KMEM2, &km2, sizeof km2) == 0;   /* (KICK 11..20, 0.9.2) */
}
/* LOAF BREAKER: the kick memory k's knobs from the ones playing (kx_put), the ones playing from k's (kx_get); kx_all:
 * every memory's as the ones playing (an older save, RESET ALL) */
static void kx_put(uint32_t k)
{
    uint8_t *x = kx_at(k % KICK_MEMS);
    x[0] = set.selk_asg;
    x[1] = set.selk_asg2;
    x[2] = set.csub;
    memcpy(x + 3, set.kbass, 4);
    x[7] = set.kmorph;
    x[8] = set.kmorph_to;
    (*kxh_at(k % KICK_MEMS)) = set.selk_hi;
}
static void kx_get(uint32_t k)
{
    const uint8_t *x = kx_at(k % KICK_MEMS);
    set.selk_asg = x[0];
    set.selk_asg2 = x[1];
    set.csub = x[2];
    memcpy(set.kbass, x + 3, 4);
    set.kmorph = x[7];
    set.kmorph_to = x[8];
    set.selk_hi = (*kxh_at(k % KICK_MEMS));
}
static void kx_all(void)
{
    uint32_t k;
    for (k = 0; k < KICK_MEMS; k++)
        kx_put(k);
}
static void km2_defaults(void)           /* (KICK 11..20 as they begin: a save from before them, 0.9.2) */
{
    uint32_t k;
    km2.magic = KM2_MAGIC;
    for (k = KMEM_A; k < KICK_MEMS; k++) {
        kick_preset(kmem_at(k), k % KICK_PRESETS);
        kmem_at(k)->volume = set.kick.volume;
        memset(ksmem_at(k), 0, KS_N);
        kx_put(k);
    }
    km2_lab_clear();
}
static uint8_t *stepb(uint32_t pi, uint32_t d, uint32_t s)   /* pattern pi, drum d (DR_*), its step s */
{
    d %= NDRUMS;
    return &steps.b[pi % NPAT][d == DR_KICK ? (s & 63u) : 64u + (d - 1u) * 32u + (s & 31u)];
}
static void steps_defaults(void)          /* (none: from the patterns' HITS) */
{
    uint32_t pi, d, s;
    steps.magic = STEPS_MAGIC;
    memset(steps.tk, 0, sizeof steps.tk);
    memset(steps.tb, 0, sizeof steps.tb);
    for (pi = 0; pi < NPAT; pi++)
        for (d = 0; d < NDRUMS; d++)
            for (s = 0; s < (d == DR_KICK ? 64u : 32u); s++) {
                uint32_t h = pats.hits[pi][d];
                *stepb(pi, d, s) = (uint8_t)(h >= 2u && h <= 4u ? h - 1u : 0u);
            }
}
_Static_assert(sizeof pats <= 3840u, "a store object holds 3840 bytes");
/* LOAF BREAKER: BUILD and DROP. SEL clicked three times, playing (build_n, the main loop): from the next bar the
 * drums' steps give way to a snare roll that speeds up (quarters, eighths, 16ths, then 32nds, a quarter of the build
 * each), louder and louder, the whole mix high-passed higher and higher under a rising noise; after its bars (DROP's
 * BUILD: 1, 2, 4, 8; none: the bar playing ends in the drop; or three clicks again: at the next bar),
 * the DROP: everything back at once, the next pattern's drums from its first step (drum_next, ready from the main
 * loop), a crash on it (drop_n counts them: the screen flashes and moves to that pattern) */
static const uint8_t DROP_BARS[5] = {0u, 1u, 2u, 4u, 8u};   /* the build's bars (DROP's K1) */
static volatile uint8_t build_bars = 4u; /* the next build's (the main loop's, as it asks for it) */
static volatile uint32_t build_len;      /* the build's steps (the render's) */
static volatile uint8_t build_wait;      /* asked for: it starts at the next bar (none: the drop at this bar's end) */
static volatile float drop_gain = 1.0f;  /* the crash's level */
static volatile uint8_t b_style, b_sweep, b_gap;   /* (the main loop's, as it asks: this build's style (DS_*, chance
                                          * picked), its filter (0 HP, 1 LP, 2 none), its gap (steps)) */
static volatile float b_riser = 1.0f;    /* its noise's level (1: as it always was) */
/* LOAF BREAKER: the roll pads' KICK ROLL (their THRU KICK or BOTH): the pad held + 1 (0 none, the main loop's); the
 * render hits the kick at that pad's rate from the next step, the pattern's kicks out meanwhile */
static volatile uint8_t kroll;
#define KSC_W 232u                       /* LOAF BREAKER TEST: the kick as it sounds, its last hit's first 250 ms, a peak a
                                          * column (KICK menu's picture, x1000) */
#define KSC_N 11025u
static volatile int16_t kscope[KSC_W];
static volatile uint32_t kscope_hit;
static volatile uint8_t kick_hold;       /* LOAF BREAKER: KIL (the tenth black key) held: the sequencer's kicks quiet */
#define LIM_LA 64u                       /* LOAF BREAKER: the output limiter's look-ahead, samples (1.5 ms) */
static inline float lim_soft(float x)    /* over 0.95: rounded off, never reaching 1 */
{
    float a = x < 0.0f ? -x : x;
    if (a <= 0.95f)
        return x;
    a = 0.95f + 0.05f * (1.0f - 1.0f / (1.0f + (a - 0.95f) * 20.0f));
    return x < 0.0f ? -a : a;
}         /* LOAF BREAKER TEST: ARP held: FILL (the steps' FILL / NOT FILL, the snare's
                                          * roll on the bar's last beat) */
static volatile uint32_t kick_hit_n;     /* (the kick's hits so far: the simulator's checks) */
/* LOAF BREAKER: the drums' HITS (pats.hits): a hit's repeats still to come, samples from this block's start */
#define HQ_N 16u
static struct { int32_t at; uint8_t kick, who; float pm; } hq[HQ_N];
static uint32_t hq_n;
static void hq_push(int32_t at, uint8_t kick, uint8_t who, float pm);
/* a drum's hit on step si of pattern pi, at off (len: a 16th): its step's CHANCE (0: skipped), its NUDGE (later) and
 * its HITS (the repeats in its step) queued; 1: it sounds at off itself */
static uint32_t step_rng = 0x9E3779B9u;
static uint32_t pat_cycle;               /* LOAF BREAKER TEST: the pattern's loops (its first step), for 1:2 and 2:2 */
static int step_hit(uint32_t pi, uint32_t d, uint32_t si, int32_t off, int32_t len, uint8_t kick, uint8_t who, float pm)
{
    uint32_t b = *stepb(pi, d, si), hn = (b & 3u) + 1u, ch = (b >> 2) & 7u, nd = b >> 5, j;
    int32_t at;
    if (ch >= 1u && ch <= 3u) {          /* LOAF BREAKER TEST: CHANCE 75 %, 50 %, 25 % */
        step_rng = step_rng * 1664525u + 1013904223u;
        if ((step_rng >> 30) < ch)
            return 0;
    } else if ((ch == 4u && (pat_cycle & 1u)) || (ch == 5u && !(pat_cycle & 1u)))   /* (1:2 the first loop of two,
                                          * 2:2 the second; FILL, NOT FILL: always, FILL gone in 0.244) */
        return 0;
    at = off + (int32_t)nd * len / 16;
    if (nd)
        hq_push(at, kick, who, pm);
    for (j = 1; j < hn; j++)
        hq_push(at + len * (int32_t)j / (int32_t)hn, kick, who, pm);
    return !nd;
}
static void hq_push(int32_t at, uint8_t kick, uint8_t who, float pm)
{
    if (hq_n < HQ_N) {
        hq[hq_n].at = at;
        hq[hq_n].kick = kick;
        hq[hq_n].who = who;
        hq[hq_n].pm = pm;
        hq_n++;
    }
}
static volatile uint8_t build_n;         /* three clicks: a build, or (building) the drop */
static volatile uint16_t drum_next[NDRUMS][4];   /* the next pattern's drums, for the drop */
static volatile uint8_t drop_to = 0xFFu; /* the pattern the drop brings in (its break's steps, at the bar before) */
static volatile uint8_t drum_next_ok;
static volatile uint8_t drop_n;
static volatile float comp_gr;           /* LOAF BREAKER: COMP's reduction now, dB (the KNOB page's meter) */
/* LOAF BREAKER: what leaves the mix (before MASTER), a column a 15th of a step (44100 / BPM samples): its lows (under
 * ~200 Hz), mids, highs (over ~2.5 kHz) and peak, 0..255, for HOME's CDJ view (cdj.c); a ring, scope_w the columns
 * written so far */
#define SCOPE_N 480u                     /* (LOAF BREAKER: two bars, for WAVE ZOOM's widest; in RAM, the pool is full) */
static uint8_t scope[SCOPE_N][4];
static volatile uint8_t scope_on = 1u;   /* (LOAF BREAKER, 0.226: the main loop's: the CDJ view on screen; else the
                                          * render spares the scope's work) */
static volatile uint32_t scope_w;
static volatile float build_p = -1.0f;   /* the build's progress 0..1, -1 none (the screen's and the lights') */
static volatile uint16_t drum_now[NDRUMS][4];   /* the pattern playing's drums, bar by bar of four (their lengths
                                          * played out: pat_lanes_out), for the render */
static int8_t pat_next = -1;             /* a pattern picked while playing: it starts at the loop's end */

static void trk_defaults(void)          /* every track empty; the first a start: P1, P2, a drop, P3 */
{
    static const uint8_t T1[5] = {1u, 2u, TI_DROP, 3u, TI_END}, R1[5] = {4u, 4u, 1u, 8u, 1u};
    uint32_t t, i;
    for (t = 0; t < NTRACKS; t++)
        for (i = 0; i < TRK_ITEMS; i++) {
            pats.trk[t].it[i] = t == 0u && i < 5u ? T1[i] : TI_END;
            pats.trk[t].rep[i] = t == 0u && i < 5u ? R1[i] : 1u;
            pats.trk[t].end = 0u;
        }
}
static void pat_defaults(void)
{
    uint32_t i, k;
    pats.magic = PAT_MAGIC;
    for (i = 0; i < NPAT; i++) {
        pat_t *p = &pats.p[i];
        for (k = 0; k < PAT_STEPS; k++) {
            p->st[k][0] = 1u;            /* follow the loop */
            p->st[k][1] = p->st[k][2] = p->st[k][3] = 0u;
        }
        p->lane[LANE_KICK] = p->lane[LANE_SNARE] = p->lane[LANE_HAT] = 0u;   /* the drums start empty */
        p->kick2 = 0u;
        p->st[6][3] = (uint8_t)(set.novice ? 4u : 0u);   /* (LOAF BREAKER: BEGINNER, the kick 16 steps (0.227-0.245:
                                          * 32; 0.246, FLEXOR: 16, as the drums): PAT_KICK16, defined below) */
    }
    trk_defaults();
    pats.mem.fx_ok = pats.mem.kfx_ok = pats.kfm_ok = 0u;
    for (i = 0; i < NPAT; i++) {
        for (k = 0; k < 13u; k++)
            pats.kn[i][k] = 0u;
        for (k = 0; k < NDRUMS_SAVED; k++)
            pats.hits[i][k] = 0u;
    }
}
/* GLOUPI BREAKS: the kick over four bars (64 steps), bar b (0..3) 16 bits: lane[LANE_KICK], kick2, and bars 3 and 4
 * in the spare bytes of steps 0-3 (st[0..3][3], low byte first) */
static uint32_t kick_bar(const pat_t *p, uint32_t b)
{
    b &= 3u;
    if (b < 2u)
        return b ? p->kick2 : p->lane[LANE_KICK];
    return (uint32_t)p->st[(b - 2u) * 2u][3] | (uint32_t)p->st[(b - 2u) * 2u + 1u][3] << 8;
}
static void kick_bar_set(pat_t *p, uint32_t b, uint32_t m)
{
    b &= 3u;
    if (b == 0u)
        p->lane[LANE_KICK] = (uint16_t)m;
    else if (b == 1u)
        p->kick2 = (uint16_t)m;
    else {
        p->st[(b - 2u) * 2u][3] = (uint8_t)m;
        p->st[(b - 2u) * 2u + 1u][3] = (uint8_t)(m >> 8);
    }
}
/* the snare over two bars: bar 1 lane[LANE_SNARE], bar 2 in the spare bytes of steps 4-5 */
static uint32_t snare_bar(const pat_t *p, uint32_t b)
{
    return b & 1u ? (uint32_t)p->st[4][3] | (uint32_t)p->st[5][3] << 8 : p->lane[LANE_SNARE];
}
static void snare_bar_set(pat_t *p, uint32_t b, uint32_t m)
{
    if (b & 1u) {
        p->st[4][3] = (uint8_t)m;
        p->st[5][3] = (uint8_t)(m >> 8);
    } else
        p->lane[LANE_SNARE] = (uint16_t)m;
}
/* the lanes' lengths (step 6's spare byte; 0, an older save's: the kick 64 steps, the snare 16) */
#define PAT_LEN(p) ((p)->st[6][3])
#define PAT_KICK32 1u                    /* the kick 32 steps (bars 1-2 again and again), else 64 */
#define PAT_SNARE32 2u                   /* the snare 32 steps (bars 1-2), else 16 (bar 1 every bar) */
#define PAT_KICK16 4u                    /* LOAF BREAKER: the kick 16 steps (bar 1 every bar), over PAT_KICK32 */
static uint32_t kick_bars(const pat_t *p)   /* the kick's bars played: 1, 2 or 4 */
{
    return PAT_LEN(p) & PAT_KICK16 ? 1u : PAT_LEN(p) & PAT_KICK32 ? 2u : 4u;
}
/* LOAF BREAKER: every drum, bar b of its two (the kick's four): the hat's second in the spare bytes of steps 7-8, the
 * clap's two in 9-12; 32 steps or 16: step 6's spare byte, bit 3 the hat, bit 4 the clap */
static const uint8_t LEN_BIT[NDRUMS] = {0u, PAT_SNARE32, 8u, 16u};   /* (bit 5: the MELODY's, 0.200-0.208) */
static uint32_t drum_at(uint32_t d, uint32_t b) { return d == DR_HAT ? 7u : 9u + (b & 1u) * 2u; }
static uint32_t drum_bar(const pat_t *p, uint32_t d, uint32_t b)
{
    uint32_t k;
    if (d == DR_KICK)
        return kick_bar(p, b);
    if (d == DR_SNARE)
        return snare_bar(p, b);
    if (d == DR_HAT && !(b & 1u))
        return p->lane[LANE_HAT];
    k = drum_at(d, b);
    return (uint32_t)p->st[k][3] | (uint32_t)p->st[k + 1u][3] << 8;
}
static void drum_bar_set(pat_t *p, uint32_t d, uint32_t b, uint32_t m)
{
    uint32_t k;
    if (d == DR_KICK)
        kick_bar_set(p, b, m);
    else if (d == DR_SNARE)
        snare_bar_set(p, b, m);
    else if (d == DR_HAT && !(b & 1u))
        p->lane[LANE_HAT] = (uint16_t)m;
    else {
        k = drum_at(d, b);
        p->st[k][3] = (uint8_t)m;
        p->st[k + 1u][3] = (uint8_t)(m >> 8);
    }
}
/* LOAF BREAKER (crossbreed): the kick's notes, one a step of its four bars (s 0..63), -8 .. +7 semitones, four bits
 * each: steps 1-38 in the spare bytes of steps 14-32 (st[13..31][3]), the rest in pats.kn (a pattern of pats.p only:
 * another copy keeps its own, kick_notes_copy) */
static uint8_t *kick_note_byte(const pat_t *p, uint32_t s)
{
    uint32_t pi = (uint32_t)(p - pats.p);
    s &= 63u;
    if (s < 38u)
        return (uint8_t *)&p->st[13u + s / 2u][3];
    return p >= pats.p && pi < NPAT ? &pats.kn[pi][(s - 38u) / 2u] : 0;
}
static int32_t kick_note(const pat_t *p, uint32_t s)
{
    const uint8_t *b = kick_note_byte(p, s);
    uint32_t v = b ? (s & 1u ? *b >> 4 : *b & 15u) : 0u;
    return v >= 8u ? (int32_t)v - 16 : (int32_t)v;
}
static void kick_note_set(pat_t *p, uint32_t s, int32_t v)
{
    uint8_t *b = kick_note_byte(p, s);
    uint32_t n = (uint32_t)(v < -8 ? -8 : v > 7 ? 7 : v) & 15u;
    if (b)
        *b = (uint8_t)(s & 1u ? (*b & 0x0Fu) | n << 4 : (*b & 0xF0u) | n);
}
static uint32_t drum_bars(const pat_t *p, uint32_t d)   /* its bars played: the kick 1, 2 or 4, the rest 1 or 2 */
{
    if (d == DR_KICK)
        return kick_bars(p);
    return PAT_LEN(p) & LEN_BIT[d] ? 2u : 1u;
}
static void pat_step_of(uint32_t pi, uint32_t k)   /* step k of pattern pi, to the engine */
{
    const uint8_t *b = pats.p[pi % NPAT].st[k];
    brk_step_t s;
    s.slice = (int8_t)(b[0] >= 2u ? (int32_t)b[0] - 2 : BRK_NO_SLICE);
    s.on = (uint8_t)(b[0] != 0u);
    s.pitch = (int8_t)b[1];
    s.flags = (uint8_t)(b[2] & 1u ? STEP_REVERSE : 0u);
    s.ratchet = (uint8_t)(((b[2] >> 1) & 3u) + 1u);
    s.prob = 100u;
    s.rsv[0] = s.rsv[1] = 0u;
    brk_step_set(k, &s);
}
static void pat_step_out(uint32_t k) { pat_step_of(set.pattern, k); }   /* step k of the pattern playing */
static void pat_lanes_out(void)
{
    const pat_t *p = &pats.p[set.pattern % NPAT];
    uint32_t l, d;
    for (d = 0; d < NDRUMS; d++)
        for (l = 0; l < 4u; l++)
            drum_now[d][l] = (uint16_t)drum_bar(p, d, l % drum_bars(p, d));
}
/* LOAF BREAKER (FLEXOR, 11 Nov 2026, 0.227): BEGINNER turned on, the group in RAM's patterns: the kick 32 steps, the
 * snare, the hat and the clap 16 (their second bars kept, for EXPERT's LENGTH) */
static void novice_lengths(void)
{
    uint32_t i;
    for (i = 0; i < NPAT; i++) {
        uint8_t *len = &PAT_LEN(&pats.p[i]), v = PAT_KICK16;   /* (0.246, FLEXOR: 16, was 32) */
        if (*len != v) {
            *len = v;
            set_dirty = 1u;
        }
    }
    pat_lanes_out();
    if (set.bkeys.bk[1] != BKP_CHOPPAH) {   /* (LOAF BREAKER, 11 Nov 2026, FLEXOR: BEGINNER, the black keys CHOPPAH's;
                                          * back to EXPERT they stay so until ARP › BLACK KEYS changes them) */
        set.bkeys.bk[1] = BKP_CHOPPAH;
        set_dirty = 1u;
    }
}
static void pat_apply(void)              /* the pattern playing, all of it to the engine */
{
    uint32_t k;
    for (k = 0; k < PAT_STEPS; k++)
        pat_step_out(k);
    pat_lanes_out();
}
static void pat_read_back(void)          /* the engine's steps into the pattern playing (after RANDOMIZE) */
{
    uint32_t k;
    for (k = 0; k < PAT_STEPS; k++) {
        brk_step_t s;
        uint8_t *b = pats.p[set.pattern % NPAT].st[k];
        brk_step_get(k, &s);
        b[0] = (uint8_t)(!s.on ? 0u : s.slice >= 0 ? (uint32_t)s.slice + 2u : 1u);
        b[1] = (uint8_t)s.pitch;
        b[2] = (uint8_t)((b[2] & 0xF8u) | (s.flags & STEP_REVERSE ? 1u : 0u) |   /* (bits 3-7: the stab's, kept) */
                         ((uint32_t)(s.ratchet ? s.ratchet - 1u : 0u) & 3u) << 1);
    }
}

/* ------------------------------------------------------------------- engine --- */
static int16_t pcm_a[CH_PCM_MAX] PLAT_POOL;
#define CH_DUB_LEN 30720u                /* the dub delay's line: 0.70 s a side, 3/8 down to 97 BPM, 1/2 to 172 */
static int16_t dub_line[CH_DUB_LEN] PLAT_POOL;
static int16_t ovl_line[2u * OVL_LINE] PLAT_POOL;   /* the overlays' WOBBLE and JET (4 KB) */
static brk_break_t brk0;
static brk_break_t brk_cur[2];           /* the bank's break playing (pcm_a holds it), and the next one's: the render
                                          * keeps its own while the next is written */
static uint32_t n_synth;                 /* 0; 1 in a build with no bank: a synthesised break (brk_synth.c) */
static uint32_t n_breaks;                /* then the bank's valid slots */
static int32_t brk_next = -1;            /* a break chosen, waiting for the next bar (break_ask), or -1 */
static uint8_t wave_px[240];             /* the waveform's height per column, for the screen */
#define BLK_N 16u                        /* the slicer view's blocks: one per white key */
#define BLK_W 26u                        /* columns of waveform in a block */
static uint8_t blk_px[BLK_N][BLK_W];     /* each slice's waveform, 0..19 */
static uint8_t blk_n;                    /* blocks with a slice behind them */
static volatile uint8_t muted;
static volatile float meter_brk, meter_kick, meter_perc;   /* the MIXER's channels: the break, the kick, the snare
                                                           * and hat, before the chain */
static volatile float meter_l, meter_r;  /* the output's peaks as it leaves (after MASTER: 1 = full scale), falling
                                          * ~20 dB a second: the render writes, the header's meter reads */
static volatile uint8_t out_sound;       /* LOAF BREAKER: a block was heard (over -60 dB, after MASTER): the render sets
                                          * it, AUTO SAVE clears it (no save while a tail, an echo, still sounds) */
#define LR_TEST_LEN (4u * BRK_SR)
static volatile uint32_t lr_test;        /* the left / right test (BT AUDIO K3 while streaming): samples left of it */

static void wave_build(const brk_break_t *b)
{
    uint32_t x, i;
    for (x = 0; x < 240u; x++) {
        uint32_t a = b->len * x / 240u, z = b->len * (x + 1u) / 240u, pk = 0u;
        for (i = a; i < z; i++) {
            int32_t s = b->pcm[i] < 0 ? -b->pcm[i] : b->pcm[i];
            if ((uint32_t)s > pk)
                pk = (uint32_t)s;
        }
        wave_px[x] = (uint8_t)(pk * 24u / 32768u);
    }
    blk_n = (uint8_t)(b->nslices < BLK_N ? b->nslices : BLK_N);
    for (x = 0; x < BLK_N; x++) {
        uint32_t a = x < blk_n ? b->slice[x] : 0u, z = x < blk_n ? b->slice[x + 1] : 0u, c;
        for (c = 0; c < BLK_W; c++) {
            uint32_t i0 = a + (z - a) * c / BLK_W, i1 = a + (z - a) * (c + 1u) / BLK_W, pk = 0u;
            for (i = i0; i < i1; i++) {
                int32_t v = b->pcm[i] < 0 ? -b->pcm[i] : b->pcm[i];
                if ((uint32_t)v > pk)
                    pk = (uint32_t)v;
            }
            blk_px[x][c] = (uint8_t)(pk * 19u / 32768u);
        }
    }
}

/* the bank built into the image: its slot i (0..), or 0 when absent or not sane */
static const bank_slot_t *bank_slot(uint32_t i)
{
#if CH_HAVE_BANK
    const bank_header_t *h = (const bank_header_t *)CH_BANK;
    const bank_slot_t *s;
    if (h->magic != BANK_MAGIC || h->version != BANK_VERSION || i >= h->nslots || i >= BANK_SLOTS ||
        16u + 168u * (i + 1u) > CH_BANK_SIZE)
        return 0;
    s = (const bank_slot_t *)(CH_BANK + 16u + 168u * i);
    if (s->data_off + s->data_len > CH_BANK_SIZE || s->len > CH_PCM_MAX || s->data_len < (s->len + 1u) / 2u ||
        !s->nslices || s->nslices > BANK_SLICES || !s->bars || s->slice[s->nslices] != s->len || !s->bpm_x100)
        return 0;
    return s;
#else
    (void)i;
    return 0;
#endif
}

/* break i's name, for the screen before it is in */
static const char *break_name(uint32_t i)
{
    static char t[13];
    const bank_slot_t *s = i >= n_synth ? bank_slot(i - n_synth) : 0;
    uint32_t k;
    for (k = 0; k < 12u; k++)
        t[k] = s ? s->name[k] : 0;
    t[12] = 0;
    return s ? t : brk0.name;
}

/* break i into pcm_a and the descriptor the render isn't using (whatever plays from pcm_a meanwhile is muted, see
 * break_poll); hand it to the render after */
static const brk_break_t *break_load(uint32_t i)
{
    const bank_slot_t *s = i >= n_synth ? bank_slot(i - n_synth) : 0;
    brk_break_t *d = brk_get_break() == &brk_cur[0] ? &brk_cur[1] : &brk_cur[0];
    const brk_break_t *b = &brk0;
    uint32_t k;
    if (s) {
        adpcm_state_t st = {0, 0, 0};
#if CH_HAVE_BANK
        adpcm_decode(&st, CH_BANK + s->data_off, pcm_a, s->len);
#endif
        d->pcm = pcm_a;
        d->len = s->len;
        d->rate = s->rate;
        d->bpm_x100 = s->bpm_x100;
        d->bars = s->bars;
        d->nslices = s->nslices;
        for (k = 0; k <= s->nslices; k++)
            d->slice[k] = s->slice[k];
        for (k = 0; k < 12u; k++)
            d->name[k] = s->name[k];
        d->name[12] = 0;
        b = d;
    } else {                             /* a built-in style, played into RAM now */
        if (i >= n_synth)
            i = 0u;
#if !CH_HAVE_BANK                        /* (a bank build has no synthesised breaks: their code stays out) */
        if (brk_synth_style_len(i, CH_RATE) <= CH_PCM_MAX)
            brk_synth_style(pcm_a, CH_RATE, i, &brk0);
#endif
    }
    set.brk = (uint8_t)i;
    wave_build(b);
    return b;
}

static int16_t rum_line[4096] PLAT_POOL;   /* LOAF BREAKER: the kick's RUMBLE, its line (93 ms) */
void app_init(void)
{
    kick_rumble_buf(rum_line, 4096u);
    brk_init();
    brk_fx_init();
    brk_fx_delay_buffer(dub_line, CH_DUB_LEN);
    kick_init();
    perc_init();
    ovl_init(ovl_line);
    n_synth = n_breaks = 0u;             /* the breaks are the bank's (breaks/breaks.txt) */
    while (n_breaks < BANK_SLOTS && bank_slot(n_breaks))
        n_breaks++;
    if (!n_breaks)                       /* a build without a bank still has a break: a synthesised Amen */
        n_synth = n_breaks = 1u;
    settings_defaults();                 /* (what a shorter, older save doesn't reach keeps these) */
    {
        int sz = plat_store_size(OBJ_SETTINGS);
        if (sz < (int)SET_SIZE_MIN || sz > (int)sizeof set || plat_store_load(OBJ_SETTINGS, &set, (uint32_t)sz) != sz)
            set.magic = 0u;
        else if (sz <= (int)offsetof(settings_t, novice))
            set.novice = 0u;             /* (TEST: a save from before the MODE: its owner knows the way, EXPERT) */
        if (set.magic == SET_MAGIC && sz <= (int)offsetof(settings_t, tut))
            set.tut = 0u;
        if (set.magic == SET_MAGIC && !(set.skin & 0x80u))   /* (LOAF BREAKER, 0.237: a save from before the 1.0's
                                          * skins: its place in the 18 before, the nearest kept) */
            set.skin = (uint8_t)(0x80u | skin_old(set.skin));
        if (set.pxv != PX_OK) {          /* (LOAF BREAKER: a save from before the drums' extras: smem the MELODY's
                                          * leftovers, cleared) */
            memset(set.smem, 0, sizeof set.smem);
            set.pxv = PX_OK;
        }
        if (set.bkeys.bk[0] != BK_TAG || set.bkeys.bk[1] >= BKP_N)
            bk_defaults();               /* (LOAF BREAKER: a save from before the BLACK KEYS: the STAB's leftovers) */
                         /* (LOAF BREAKER: a save from before the TUTORIAL: an FM-1 in use, none started;
                                          * SETUP K1 starts it) */
    }
    if (set.magic != SET_MAGIC || set.tempo_x10 < 400u || set.tempo_x10 > 3000u || set.swing < 50u ||
        set.swing > 75u) {
        settings_defaults();
        if (n_breaks > n_synth)
            set.brk = (uint8_t)n_synth;  /* the bank's first break */
    }
    if (set.brk >= n_breaks)
        set.brk = 0u;
    trk_defaults();
    pats.mem.fx_ok = pats.mem.kfx_ok = pats.kfm_ok = 0u;   /* (an older save: no effect remembered) */
    {
        int sz = plat_store_size(OBJ_PAT);   /* (LOAF BREAKER: a save from before the tracks: its patterns) */
        if ((sz != (int)sizeof pats && sz != (int)PATS_SIZE_OLD && sz != (int)PATS_SIZE_TRK &&
             sz != (int)PATS_SIZE_MEM && sz != (int)PATS_SIZE_KFM && sz != (int)PATS_SIZE_KN) ||
            plat_store_load(OBJ_PAT, &pats, (uint32_t)sz) != sz)
            pats.magic = 0u;
    }
    if (pats.magic != PAT_MAGIC && pats.magic != PAT2_MAGIC && pats.magic != PAT1_MAGIC)
        pat_defaults();
    if (plat_store_load(OBJ_KLAB, &klab, sizeof klab) != (int)sizeof klab || klab.magic != KLAB_MAGIC)
        klab_defaults();                 /* (LOAF BREAKER: a save before the KICK LAB: every kick as it sounded) */
    {                                    /* (LOAF BREAKER, 0.9.2: KICK 11..20; a save before them: as they begin; the
                                          * KICK LAB's line above may have cleared their LAB: theirs read after) */
        km2_lab_clear();
        if (plat_store_load(OBJ_KMEM2, &km2, sizeof km2) != (int)sizeof km2 || km2.magic != KM2_MAGIC)
            km2_defaults();
    }
    int ssz = plat_store_load(OBJ_STEP, &steps, sizeof steps), sok;
    sok = (ssz == (int)sizeof steps || ssz == (int)STEPS_SIZE_KXH || ssz == (int)STEPS_SIZE_TK || ssz == (int)STEPS_SIZE_KX ||
           ssz == (int)STEPS_SIZE_TB ||
           ssz == (int)STEPS_SIZE_OLD) && steps.magic == STEPS_MAGIC;
    if (!sok)
        steps_defaults();                /* (LOAF BREAKER: the drum steps' own settings) */
    else if (ssz == (int)STEPS_SIZE_OLD)
        memset(steps.tb, 0, sizeof steps.tb);   /* (a save before the tracks' breaks) */
    if (!sok || ssz < (int)STEPS_SIZE_KX)
        kx_all();                        /* (a save before the kicks' own knobs: every kick with the ones it had) */
    if (!sok || ssz < (int)STEPS_SIZE_TK)
        memset(steps.tk, 0, sizeof steps.tk);   /* (a save before the items' kicks: the one playing) */
    if (!sok || ssz < (int)STEPS_SIZE_KXH)
        memset(steps.kxh, 0, sizeof steps.kxh);   /* (a save before the knobs past 15: none) */
    if (!sok || ssz != (int)sizeof steps)
        memset(steps.kpre, 0, sizeof steps.kpre);   /* (a save before STYLE's presets: none stored) */
    if (sok && pats.magic != PAT_MAGIC) {   /* an older save: its kick again over four bars, as it played */
        uint32_t i;
        for (i = 0; i < NPAT; i++) {
            pat_t *p = &pats.p[i];
            if (pats.magic == PAT1_MAGIC)    /* CHOPPAH: one bar */
                p->kick2 = p->lane[LANE_KICK];
            kick_bar_set(p, 2u, kick_bar(p, 0u));
            kick_bar_set(p, 3u, kick_bar(p, 1u));
        }
        pats.magic = PAT_MAGIC;
    }
    set.pgrp %= NGRP;                    /* LOAF BREAKER: the group saved with its own objects: those; none (a save from
                                          * before the 64): OBJ_PAT's, written to its own now (the sound not started) */
    if (grp_has(set.pgrp))
        grp_load(set.pgrp);
    else {
        grp_sum = grp_hash();
        grp_new = (uint8_t)(plat_store_size(OBJ_PAT) > 0);
        if (grp_new)
            (void)grp_save();
    }
    brk_set_break(break_load(set.brk));  /* boot: the audio isn't running yet */
    settings_apply();
    pat_apply();
}

/* ------------------------------------------------------- MIDI clock, Bluetooth --- */
#define PKT_RT(b) (0x0Fu | (uint32_t)(b) << 8)   /* a real-time byte as a USB-MIDI packet (CIN 0xF) */
#define MIDI_SLICE0 48u                  /* C3: slice 1 */
static void midi_slice(int s, int on);   /* (with the UI: they play as the keys, and REC records them) */
static void midi_drum(uint32_t lane);
#define MS_PER_SAMPLE (1000.0f / (float)BRK_SR)
#define CLKQ 64u
/* (times in plat_ms: the 1 kHz tick sends at that grain anyway, a beat of ticks averages the tempo, and the host
 * simulator keeps plat_ms in its own time) */
static volatile uint32_t clkq_due[CLKQ]; /* clock out: when each tick is due (plat_ms); the render writes them */
static volatile uint32_t clkq_w, clkq_r; /* and the 1 kHz tick sends them */
static volatile uint32_t sync_n16, sync_ms, sync_c;   /* clock in: the outside 16th begun, when, and a count */
static uint32_t sync_seen;
#define EXT_FIT 96u                      /* GLOUPI BREAKS: the ticks the tempo is fitted over, four beats */
static struct {
    uint32_t ring[EXT_FIT], i;           /* the last ticks' stamps (plat_ticks24's); ticks in the ring's run */
    uint32_t last, n;                    /* the last tick; ticks since the start (FA) */
    float bpm, used;                     /* the tempo they give, smoothed; the one the engine has */
    uint8_t run, port;                   /* started (FA / FB, not yet FC); where they come from */
} ext;

/* GLOUPI BREAKS: the clock's tempo and the newest tick's time, from a straight line through the ring's ticks (least
 * squares). The ticks are read in the main loop, late now and then by a screen drawn meanwhile (several at once): one
 * tick's time against another's a beat before swung the tempo by 2 BPM; the line through four beats of them barely
 * moves. LOAF BREAKER: the ring holds where each tick landed (plat_midi_in_t24, fm1.toml midi_stamp: 24 MHz, within
 * 0.05 ms of its landing), the line through them steadier still. Returns 0 until there are two beats' worth; *at: the
 * newest tick as the line has it (ms, from t24, the newest's own stamp) */
static int ext_fit(uint32_t t24, float *bpm, int32_t *at)
{
    uint32_t m = ext.i < EXT_FIT ? ext.i : EXT_FIT, k;
    float sx = 0.0f, sy = 0.0f, sxx = 0.0f, sxy = 0.0f, n, per;
    if (m < 48u)
        return 0;
    for (k = 0; k < m; k++) {            /* x: the tick, oldest 0; y: its time, ms before the newest */
        float x = (float)k, y = -(float)(t24 - ext.ring[(ext.i - m + k) % EXT_FIT]) * (1.0f / 24000.0f);
        sx += x;
        sy += y;
        sxx += x * x;
        sxy += x * y;
    }
    n = (float)m;
    per = (sxy - sx * sy / n) / (sxx - sx * sx / n);   /* ms a tick */
    if (per <= 0.0f)
        return 0;
    *bpm = 2500.0f / per;                /* 24 ticks a beat: 60000 / (24 per) */
    *at = (int32_t)(sy / n + per * ((n - 1.0f) - sx / n) + (sy < 0.0f ? -0.5f : 0.5f));
    return 1;
}

static uint32_t midi_dest(void)
{
    return set.midi_to == 1u ? TO_USB : set.midi_to == 2u ? TO_BLE : TO_USB | TO_BLE;
}

static int ext_active(void)              /* following an outside clock: on, and ticks in the last half second */
{
    return set.clk_in && ext.i && plat_ms() - ext.last < 500u;
}

/* PLAY: the transport, and with clock out on, MIDI start / stop after it. HOME + PLAY (restart): back to bar 1,
 * and while playing a start again */
static void transport(int on, int restart)
{
    if (restart)
        brk_reset();
    else
        brk_play(on);
    if (set.clk_out && (on || !restart))
        plat_midi_send(PKT_RT(on ? 0xFAu : 0xFCu), midi_dest());
}

int app_ble_at_boot(void) { return !set.ble_off; }

/* main loop, every pass: MIDI in. With clock in on, ticks set the tempo (a beat's worth, smoothed) and every beat
 * the loop's phase (brk_sync, in the render); start, continue and stop drive the transport */
void app_loop(void)
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = pkt >> 8 & 0xFFu, t = plat_ms(), t24 = plat_midi_in_t24();   /* (t24: where it landed) */
        if ((st & 0xE0u) == 0x80u) {     /* notes, any channel: 48..63 (C3..D#4) the slices, held as the white keys;
                                          * 36 the kick, 38 the snare, 42 / 44 / 46 the hat (a drum machine's) */
            uint32_t note = pkt >> 16 & 0x7Fu, on = (st & 0xF0u) == 0x90u && (pkt >> 24 & 0x7Fu);
            if (note >= MIDI_SLICE0 && note < MIDI_SLICE0 + 16u)
                midi_slice((int)(note - MIDI_SLICE0), (int)on);
            else if (on && (note == 36u || note == 38u || note == 39u || note == 42u || note == 44u || note == 46u))
                midi_drum(note == 36u ? DR_KICK : note == 38u ? DR_SNARE : note == 39u ? DR_CLAP : DR_HAT);   /* (LOAF
                                          * BREAKER: 39 the clap) */
            continue;
        }
        if (!set.clk_in)
            continue;
        if (st == 0xF8u) {
            float bpm;
            int32_t at = 0;
            if (ext.i && t - ext.last > 250u)   /* the clock gone a while (stopped, unplugged): a fresh fit */
                ext.i = 0u;
            ext.ring[ext.i % EXT_FIT] = t24;
            ext.i++;
            ext.last = t;
            ext.port = (uint8_t)port;
            if (ext_fit(t24, &bpm, &at) && bpm >= 40.0f && bpm <= 300.0f) {
                float d = bpm - ext.bpm;     /* eased; a real change (over 2 BPM) followed at once */
                ext.bpm = ext.bpm > 0.0f ? ext.bpm + d * (d > 2.0f || d < -2.0f ? 0.5f : 0.05f) : bpm;
                d = ext.bpm - ext.used;
                if (d >= 0.05f || d <= -0.05f) {   /* the engine's tempo moved only past a twentieth of a BPM */
                    ext.used = ext.bpm;
                    brk_tempo(ext.bpm);
                    brk_fx_tempo(ext.bpm);
                    set.tempo_x10 = (uint16_t)(ext.bpm * 10.0f + 0.5f);   /* (shown; not a change to save) */
                }
            } else
                at = 0;
            if (ext.run) {               /* each beat: where the outside clock is, for the render */
                if (ext.n % 24u == 0u) {
                    sync_n16 = ext.n / 6u;
                    sync_ms = t - (plat_ticks24() - t24) / 24000u + (uint32_t)at;   /* (the beat's time as the
                                          * line has it, from where it landed: not when it was read) */
                    sync_c++;
                }
                ext.n++;
            }
        } else if (st == 0xFAu || st == 0xFBu) {   /* start (continue starts again too: no song position yet) */
            ext.n = 0u;
            ext.run = 1u;
            brk_play(0);
            brk_play(1);
            muted = 0u;
        } else if (st == 0xFCu) {
            ext.run = 0u;
            brk_play(0);
        }
    }
}

/* 1 kHz: the clock ticks the render marked, each sent at its moment */
void app_tick(void)
{
    uint32_t now = plat_ms();
    while (clkq_r != clkq_w && (int32_t)(now - clkq_due[clkq_r % CLKQ]) >= 0) {
        plat_midi_send(PKT_RT(0xF8u), midi_dest());
        clkq_r++;
    }
}

/* a meter: this block's peak, or the last one falling (about 20 dB a second) */
static float meter(const float *x, uint32_t n, float last)
{
    float pk = 0.0f;
    uint32_t i;
    for (i = 0; i < n; i++) {
        float a = x[i] < 0.0f ? -x[i] : x[i];
        if (a > pk)
            pk = a;
    }
    last *= 0.987f;
    return pk > last ? pk : last;
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    /* (LOAF BREAKER: the block's buffers in the pool, 10 KB of RAM freed: the pool is RAM as fast) */
    static float mono[APP_BLOCK] PLAT_POOL, l[APP_BLOCK] PLAT_POOL, r[APP_BLOCK] PLAT_POOL, echo[APP_BLOCK] PLAT_POOL,
                 verb[APP_BLOCK] PLAT_POOL;
    static float kick[APP_BLOCK] PLAT_POOL, perc[APP_BLOCK] PLAT_POOL;
    static float kl[APP_BLOCK] PLAT_POOL, kr[APP_BLOCK] PLAT_POOL;   /* LOAF BREAKER: the kick through the overlays
                                          * alone (THRU KICK) */
    static float spd[APP_BLOCK] PLAT_POOL, tape0 = 1.0f;   /* LOAF BREAKER: the VINYL BREAK's speed on the drums */
    static uint8_t b_on, b_drop, b_rew;  /* LOAF BREAKER: building; the drop asked; the drop armed for the next
                                          * step (the engine rewound to the pattern's first: brk_rewind) */
    static uint32_t b_steps;             /* steps built */
    static int32_t half_due = -1;        /* the roll's 32nd between two steps: samples from this block's start */
    static float crash;                  /* the drop's crash, its level */
    static uint8_t gap_now;              /* LOAF BREAKER: the GAP before the drop: silent now */
    int vin = (set.vinyl & 3u) != 1u;    /* (THRU BOTH or KICK: the drums slow too) */
    uint32_t thru = (set.ovl_pad >> 6) & 3u;   /* the ARP key's THRU: 0 BOTH, 1 BREAK, 2 KICK */
    int kick_dry = thru != 0u;           /* the kick apart from the chain (BREAK: around the overlays too) */
    float gain = muted ? 0.0f : (float)master_q12 * (1.0f / 4096.0f) * 8300000.0f *
                 brk_exp2f(-(float)max_out_db() * 0.16610f);   /* (LOAF BREAKER: MAX OUT's headroom) */
    brk_status_t st;
    uint32_t t0 = plat_ms(), done = 0u, total = n;
    if (sync_c != sync_seen) {           /* the outside clock's beat, and how long ago it came */
        float age = (float)(t0 - sync_ms) / MS_PER_SAMPLE;
        sync_seen = sync_c;
        if (age < (float)BRK_SR * 0.25f)
            brk_sync(sync_n16, age);
    }
    while (n) {
        uint32_t m = n < APP_BLOCK ? n : APP_BLOCK, i, ngev = 0u;
        uint16_t gev_at[4];              /* (the GAP: where it begins or ends in this block) */
        uint8_t gev_v[4], gap_was = gap_now;
        brk_sends(echo, verb);           /* the slices' throws into the delay and the reverb */
        brk_render(mono, m);
        brk_status(&st);
        {                                /* the MIXER's BREAK fader (eased: no zipper), on the break and its throws */
            static float lvl = 1.0f;
            float want = (float)set.brk_level * 0.01f;
            for (i = 0; i < m; i++) {
                lvl += (want - lvl) * 0.002f;
                mono[i] *= lvl;
                echo[i] *= lvl;
                verb[i] *= lvl;
            }
            meter_brk = meter(mono, m, meter_brk);
        }
        if (set.clk_out) {               /* clock out: each tick due as its sample plays (a block from now) */
            uint16_t off[16];
            uint32_t k, nt = brk_clock_ticks(off, 16u);
            for (k = 0; k < nt && clkq_w - clkq_r < CLKQ; k++) {
                clkq_due[clkq_w % CLKQ] = t0 + (uint32_t)((float)(done + off[k] + total) * MS_PER_SAMPLE + 0.5f);
                clkq_w++;
            }
        }
        done += m;
        {                                /* the drums: on the steps of the bar their lanes have, at their samples */
            uint16_t off[8], ka[8], pa[8];
            float kpm[8];                /* (crossbreed: each kick's note, a factor) */
            uint8_t idx[8], who[8];
            uint32_t k, ne = brk_step_events(off, idx, 8u), nk = 0u, np = 0u;
            static int32_t kr_next = -1;     /* (the KICK ROLL's next hit, samples from this block's start; -1:
                                          * waiting for a step) */
            static uint8_t kr_seen;
            uint32_t krp = kroll;
            {                            /* LOAF BREAKER: three clicks: a build from the next bar (none: the drop
                                          * at this bar's end), or (building) the drop at the bar */
                static uint8_t seen;
                if (build_n != seen) {
                    seen = build_n;
                    if (b_on)
                        b_drop = 1u;
                    else if (st.playing)
                        build_wait = 1u;
                }
            }
            if (!st.playing)
                b_on = b_drop = b_rew = build_wait = gap_now = 0u;   /* (stopped: no build) */
            if (b_on && half_due >= 0 && half_due < (int32_t)m) {   /* (the 32nd between two steps) */
                pa[np] = (uint16_t)half_due;
                who[np++] = 1u << PV_SNARE;
            }
            if (half_due >= 0 && (half_due -= (int32_t)m) < 0)
                half_due = -1;
            if (krp != kr_seen) {        /* (a roll pad down or up: from its next step) */
                kr_seen = (uint8_t)krp;
                kr_next = -1;
            }
            for (k = 0; k < ne; k++) {
                uint32_t s = idx[k] & 15u;   /* the step's place in the bar (idx: in four bars, the kick's) */
                if (!idx[k])
                    pat_cycle++;
                if (build_wait && s == 0u && build_bars) {   /* the build asked for: from this bar's first step */
                    build_wait = 0u;
                    b_on = 1u;
                    b_drop = b_rew = 0u;
                    b_steps = 0u;
                    build_len = 16u * build_bars;
                    half_due = -1;
                }
                if (b_on) {              /* LOAF BREAKER: the build's roll in the drums' place */
                    if (b_rew && s == 0u) {   /* the drop: the new pattern's first step (rewound at the bar's
                                          * last), everything back, its drums at once */
                        uint32_t dd, bb;
                        if (gap_now && ngev < 4u) {   /* (the GAP ends with the drop) */
                            gev_at[ngev] = off[k];
                            gev_v[ngev++] = 0u;
                            gap_now = 0u;
                        }
                        b_on = b_drop = b_rew = 0u;
                        half_due = -1;
                        if (drum_next_ok)
                            for (dd = 0; dd < NDRUMS; dd++)
                                for (bb = 0; bb < 4u; bb++)
                                    drum_now[dd][bb] = drum_next[dd][bb];
                        drum_next_ok = 0u;
                        drop_n++;
                        crash = drop_gain;
                    } else if (build_len) {
                        uint32_t q4 = b_steps * 4u / build_len, div = q4 < 1u ? 4u : q4 < 2u ? 2u : 1u, q;
                        uint32_t sty = b_style, rem = b_drop ? 16u - s : build_len - b_steps;
                        if (b_gap && rem <= b_gap && !gap_now && ngev < 4u) {   /* (LOAF BREAKER: the GAP: silence
                                          * till the drop) */
                            gev_at[ngev] = off[k];
                            gev_v[ngev++] = 1u;
                            gap_now = 1u;
                        }
                        if (!(b_steps % div) && sty <= DS_BOTH) {   /* (quarters, eighths, 16ths, 32nds: a quarter
                                          * each; LOAF BREAKER: the snare's, the kick's, or the two in turn) */
                            int kick = sty == DS_KICK || (sty == DS_BOTH && !((b_steps / div) & 1u));
                            if (kick && nk < 8u) {
                                kpm[nk] = 1.0f;
                                ka[nk++] = off[k];
                            } else if (!kick && np < 8u) {
                                pa[np] = off[k];
                                who[np++] = 1u << PV_SNARE;
                                if (q4 >= 3u)
                                    half_due = (int32_t)off[k] + (int32_t)((float)BRK_SR * 7.5f / brk_get_tempo());
                            }
                        }
                        if (b_steps < build_len)
                            b_steps++;
                        if (s == 15u && (b_drop || b_steps >= build_len)) {   /* the bar's last step: the next
                                          * is the drop, the new pattern's step 1 (its break's steps in now) */
                            b_rew = 1u;
                            brk_rewind();
                            if (drop_to < NPAT)
                                for (q = 0; q < PAT_STEPS; q++)
                                    pat_step_of(drop_to, q);
                        }
                        if (sty != DS_SWEEP)     /* (SWEEP: the drums play on under it) */
                            continue;
                    }
                } else if (build_wait && !build_bars && s == 15u) {   /* no build: this bar plays out, the drop
                                          * on the next step (the new pattern's first) */
                    uint32_t q;
                    build_wait = 0u;
                    b_on = b_rew = 1u;
                    b_drop = 0u;
                    build_len = 0u;
                    brk_rewind();
                    if (drop_to < NPAT)
                        for (q = 0; q < PAT_STEPS; q++)
                            pat_step_of(drop_to, q);
                }
                if (krp && kr_next < 0)  /* (the KICK ROLL: from this step) */
                    kr_next = (int32_t)off[k];
                uint32_t w = 0u, d, bar = idx[k] >> 4 & 3u;
                for (d = DR_SNARE; d < NDRUMS; d++)   /* (LOAF BREAKER: every drum, its bar) */
                    w |= (uint32_t)(drum_now[d][bar] >> s & 1u) << (d - 1u);
                if (drum_now[DR_KICK][bar] >> s & 1u && !krp) {   /* (a KICK ROLL: its hits instead) */
                    const pat_t *pp = &pats.p[set.pattern % NPAT];   /* (its note: this step of its bar) */
                    uint32_t ks = (bar % drum_bars(pp, DR_KICK)) * 16u + s;
                    int32_t kn = kick_note(pp, ks);
                    float f = kn ? brk_exp2f((float)kn * (1.0f / 12.0f)) : 1.0f;
                    if (step_hit(set.pattern, DR_KICK, ks, (int32_t)off[k],   /* (LOAF BREAKER: the step's CHANCE,
                                          * NUDGE, HITS) */
                                 (int32_t)((float)BRK_SR * 15.0f / brk_get_tempo()), 1u, 0u, f) && nk < 8u) {
                        kpm[nk] = f;
                        ka[nk++] = off[k];
                    }
                }
                if (w && np < 8u) {      /* (LOAF BREAKER: each drum's step, its CHANCE, NUDGE, HITS) */
                    uint32_t dd2, now = 0u;
                    int32_t len = (int32_t)((float)BRK_SR * 15.0f / brk_get_tempo());
                    const pat_t *pp = &pats.p[set.pattern % NPAT];
                    for (dd2 = DR_SNARE; dd2 < NDRUMS; dd2++)
                        if (w >> (dd2 - 1u) & 1u &&
                            step_hit(set.pattern, dd2, (bar % drum_bars(pp, dd2)) * 16u + s, (int32_t)off[k], len, 0u,
                                     (uint8_t)(1u << (dd2 - 1u)), 1.0f))
                            now |= 1u << (dd2 - 1u);
                    if (now) {
                        pa[np] = off[k];
                        who[np++] = (uint8_t)now;
                    }
                }
            }
            {                            /* LOAF BREAKER: the HITS due in this block, in with the steps' (in order) */
                uint32_t q = 0u, a, b;
                if (!st.playing)
                    hq_n = 0u;
                while (q < hq_n) {
                    if (hq[q].at < (int32_t)m) {
                        if (hq[q].kick && nk < 8u) {
                            kpm[nk] = hq[q].pm;
                            ka[nk++] = (uint16_t)(hq[q].at < 0 ? 0 : hq[q].at);
                        } else if (!hq[q].kick && np < 8u) {
                            pa[np] = (uint16_t)(hq[q].at < 0 ? 0 : hq[q].at);
                            who[np++] = hq[q].who;
                        }
                        hq[q] = hq[--hq_n];
                    } else {
                        hq[q].at -= (int32_t)m;
                        q++;
                    }
                }
                for (a = 1; a < nk; a++)         /* (the kick's hits rising) */
                    for (b = a; b > 0u && ka[b - 1u] > ka[b]; b--) {
                        uint16_t to = ka[b];
                        float tp = kpm[b];
                        ka[b] = ka[b - 1u];
                        kpm[b] = kpm[b - 1u];
                        ka[b - 1u] = to;
                        kpm[b - 1u] = tp;
                    }
                for (a = 1; a < np; a++)         /* (the others' rising) */
                    for (b = a; b > 0u && pa[b - 1u] > pa[b]; b--) {
                        uint16_t to = pa[b];
                        uint8_t tw = who[b];
                        pa[b] = pa[b - 1u];
                        who[b] = who[b - 1u];
                        pa[b - 1u] = to;
                        who[b - 1u] = tw;
                    }
            }
            if (kick_hold)
                nk = 0u;                 /* LOAF BREAKER: KIL held: the sequencer's kicks quiet (a roll still plays) */
            if (krp && st.playing) {     /* the KICK ROLL's hits in this block, at its pad's rate */
                static const float RS[BRK_ROLL_DIVS] = {4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 4.0f / 3.0f, 2.0f / 3.0f,
                                                        1.0f / 3.0f};   /* (16ths: brk.c's ROLL_STEPS) */
                int32_t per = (int32_t)(RS[set.pads.roll_div[(krp - 1u) % 3u] % BRK_ROLL_DIVS] * (float)BRK_SR * 15.0f /
                                        brk_get_tempo());
                nk = 0u;                 /* (the roll, alone, in order) */
                while (kr_next >= 0 && kr_next < (int32_t)m && nk < 8u && per > 0) {
                    kpm[nk] = 1.0f;
                    ka[nk++] = (uint16_t)kr_next;
                    kr_next += per;
                }
                if (kr_next >= (int32_t)m)
                    kr_next -= (int32_t)m;
            }
            if (vin && (st.pads >> PAD_TAPESTOP & 1u))
                nk = np = 0u;            /* the vinyl held: the drums' steps quiet, as the break's */
            int vs = vin && (tape0 < 1.0f || st.tape < 1.0f);
            if (vs)
                for (i = 0; i < m; i++)  /* (the engine's ramp, this block's end; between, straight) */
                    spd[i] = tape0 + (st.tape - tape0) * (float)(i + 1u) / (float)m;
            tape0 = st.tape;
            kick_speed(vs ? spd : 0);
            perc_speed(vs ? spd : 0);
            kick_hit_n += nk;
            kick_pitches(kpm);
            kick_set_beat((float)BRK_SR * 60.0f / brk_get_tempo());   /* (TEST: the REVERSE BASS's beat) */
            kick_render(kick, m, ka, nk);
            {                            /* (TEST: the kick's scope: from its hit, 250 ms) */
                static uint32_t ki = KSC_N;
                uint32_t i0 = 0u, c;
                if (nk) {
                    ki = 0u;
                    i0 = ka[nk - 1u];
                    for (c = 0; c < KSC_W; c++)
                        kscope[c] = 0;
                    kscope_hit++;
                }
                for (i = i0; i < m && ki < KSC_N; i++, ki++) {
                    int32_t v = (int32_t)(kick[i] * 1000.0f);
                    c = ki * KSC_W / KSC_N;
                    if ((v < 0 ? -v : v) > (kscope[c] < 0 ? -kscope[c] : kscope[c]))
                        kscope[c] = (int16_t)(v > 32000 ? 32000 : v < -32000 ? -32000 : v);
                }
            }
            brk_fx_kick_eq(kick, m);    /* LOAF BREAKER: HOME's KICK EQ */
            brk_fx_kick(kick, m);        /* and its KICK FX */
            perc_render(perc, m, pa, who, np);
            if (set.duck) {              /* LOAF BREAKER (crossbreed): DUCK: the break (and its throws; DRUMS: the
                                          * other drums too) pushed down at each kick, in its attack, back up over its
                                          * release (KNOB) */
                static float dt, de;
                float depth = (float)(set.duck > 100u ? 100u : set.duck) * 0.01f;
                float da = 1.0f / (1.0f + att_ms(set.duck_att) * 44.1f), dr = 1.0f - 1.0f / (rel_ms(set.duck_rel) * 44.1f);
                int dp = set.duck_what == 1u;
                uint32_t j = 0u;
                for (i = 0; i < m; i++) {
                    float g;
                    while (j < nk && ka[j] <= i) {
                        dt = 1.0f;
                        j++;
                    }
                    de += (dt - de) * da;
                    dt *= dr;
                    g = 1.0f - depth * de;
                    mono[i] *= g;
                    echo[i] *= g;
                    verb[i] *= g;
                    if (dp)
                        perc[i] *= g;
                }
            }
            brk_fx_drums_dj(kick, perc, m, (float)set.kick_dj * 0.01f);   /* HOME's DRUM FLT (LOAF BREAKER: every drum,
                                          * not the break) */
            build_p = b_on ? (build_len ? (float)b_steps / (float)build_len : 1.0f) : -1.0f;
            if (b_on && build_len) {     /* (the roll louder as it builds) */
                float g = 0.35f + 0.65f * build_p;
                for (i = 0; i < m; i++)
                    perc[i] *= g;
            }
            meter_kick = meter(kick, m, meter_kick);
            meter_perc = meter(perc, m, meter_perc);
            if (!kick_dry)               /* (LOAF BREAKER: THRU BREAK or KICK, the kick joins after the chain) */
                for (i = 0; i < m; i++)
                    kick[i] += perc[i];
        }
        /* the break through the DJ filter, the drums joining it into the chain (or around it) */
        {
            const float *drums = kick_dry ? perc : kick;   /* (the kick apart: the snare and the hat alone here) */
            brk_fx_process(mono, set.drums_chain ? drums : 0, echo, verb, set.drums_chain ? 0 : drums, l, r, m,
                           st.pads);
        }
        {                                /* the overlays (ARP's white keys), on the whole mix: on the engine's beat while
                                          * playing, a clock of their own stopped */
            static float beat;
            float spb = (float)BRK_SR * 600.0f / (float)(set.tempo_x10 ? set.tempo_x10 : 1700u);
            if (st.playing)
                beat = (float)(st.bar % 64u) * 4.0f + ((float)(st.bar16 < 0 ? 0 : st.bar16) + st.phase) * 0.25f;
            if (thru == 2u) {            /* LOAF BREAKER: THRU KICK, the overlays on the kick alone, the rest clean */
                for (i = 0; i < m; i++)
                    kl[i] = kr[i] = kick[i];
                ovl_process(kl, kr, m, beat, spb);
                brk_fx_add_dry(l, r, kl, kr, m);
            } else {
                ovl_process(l, r, m, beat, spb);
                if (kick_dry)            /* THRU BREAK: the kick around the chain and the overlays, clean */
                    brk_fx_add_dry(l, r, kick, kick, m);
            }
            if (!st.playing) {
                beat += (float)m / spb;
                if (beat >= 256.0f)
                    beat -= 256.0f;
            }
        }
        if ((b_on && build_len) || crash > 1e-4f) {   /* LOAF BREAKER: the build's high-pass and riser over
                                          * everything; the drop's crash */
            static float h1l, h1r, h2l, h2r, cl;
            static uint32_t nz = 0x1234567u;
            int bu = b_on && build_len, sw = b_sweep;   /* (LOAF BREAKER: its filter: 0 HP, 1 LP, 2 none) */
            float p = bu ? build_p : 0.0f, rz = 0.22f * p * p * b_riser;
            float kk = brk_clampf(sw == 1 ? 6.2832f * 18000.0f * brk_exp2f(-p * 5.5f) / (float)BRK_SR
                                          : 6.2832f * 30.0f * brk_exp2f(p * 6.5f) / (float)BRK_SR, 0.0f, 0.9f);
            for (i = 0; i < m; i++) {
                float n;
                nz = nz * 1664525u + 1013904223u;
                n = (float)(int32_t)nz * (1.0f / 2147483648.0f);
                if (bu && sw == 0) {     /* (30 Hz up to ~2.7 kHz, two poles) */
                    float xl = l[i] + n * rz, xr = r[i] + n * rz;
                    h1l += (xl - h1l) * kk;
                    xl -= h1l;
                    h2l += (xl - h2l) * kk;
                    l[i] = xl - h2l;
                    h1r += (xr - h1r) * kk;
                    xr -= h1r;
                    h2r += (xr - h2r) * kk;
                    r[i] = xr - h2r;
                } else if (bu && sw == 1) {   /* (LOW-PASS: 18 kHz down to ~400 Hz, two poles) */
                    h1l += (l[i] + n * rz - h1l) * kk;
                    h2l += (h1l - h2l) * kk;
                    l[i] = h2l;
                    h1r += (r[i] + n * rz - h1r) * kk;
                    h2r += (h1r - h2r) * kk;
                    r[i] = h2r;
                } else if (bu) {         /* (no filter: the noise alone) */
                    l[i] += n * rz;
                    r[i] += n * rz;
                }
                if (crash > 1e-4f) {     /* the crash: noise above ~4 kHz, a long fall */
                    cl += (n - cl) * 0.45f;
                    l[i] += (n - cl) * crash * 0.3f;
                    r[i] += (n - cl) * crash * 0.3f;
                    crash *= 0.99993f;
                }
            }
        }
        if (lr_test) {                   /* the left / right test: a low beep on the left, then a high one on the
                                          * right, twice (4 s), over everything else */
            static float ph;
            uint32_t left = lr_test;
            for (i = 0; i < m; i++) {
                uint32_t t = LR_TEST_LEN - left, seg = t / (LR_TEST_LEN / 4u);   /* 0 L, 1 R, 2 L, 3 R */
                float on = (t % (LR_TEST_LEN / 4u)) < (LR_TEST_LEN / 4u) * 3u / 4u ? 0.5f : 0.0f;
                float x = on * brk_sin_turns(ph);
                ph += (seg & 1u ? 880.0f : 440.0f) / (float)BRK_SR;
                if (ph >= 1.0f)
                    ph -= 1.0f;
                l[i] = seg & 1u ? 0.0f : x;
                r[i] = seg & 1u ? x : 0.0f;
                if (left > 1u)
                    left--;
            }
            lr_test = left > 1u ? left : 0u;
        }
        if (gap_was || ngev) {           /* LOAF BREAKER: the GAP, silence (in ~1 ms) till the drop, the crash on it */
            static float gg = 1.0f;
            uint32_t j = 0u, on = gap_was;
            for (i = 0; i < m; i++) {
                while (j < ngev && gev_at[j] <= i)
                    on = gev_v[j++];
                gg += ((on ? 0.0f : 1.0f) - gg) * 0.02f;
                l[i] *= gg;
                r[i] *= gg;
            }
        }
        if (set.comp) {                  /* LOAF BREAKER: COMP (SEL + K3, ENV's KNOB): the whole mix over its threshold
                                          * (0 .. -36 dB) brought down by its ratio, the level followed both sides at
                                          * once (its attack, its release), half the reduction made up; then held
                                          * under full scale */
            static float cenv;
            float thr = -0.36f * (float)(set.comp > 100u ? 100u : set.comp), sl = 1.0f - 1.0f / COMP_RATIO[set.comp_ratio & 7u];
            float ca = 1.0f / (1.0f + att_ms(set.comp_att) * 44.1f), cr = 1.0f / (1.0f + rel_ms(set.comp_rel) * 44.1f);
            float mk = -thr * sl * 0.5f, gmax = 0.0f;
            for (i = 0; i < m; i++) {
                float a = brk_absf(l[i]) > brk_absf(r[i]) ? brk_absf(l[i]) : brk_absf(r[i]), over, gr, g;
                cenv += (a - cenv) * (a > cenv ? ca : cr);
                over = cenv > 1e-5f ? 6.0206f * brk_log2f(cenv) - thr : -1.0f;
                gr = over > 0.0f ? over * sl : 0.0f;
                if (gr > gmax)
                    gmax = gr;
                g = brk_exp2f((mk - gr) * 0.16610f);
                l[i] *= g;               /* (held under full scale by the limiter after) */
                r[i] *= g;
            }
            comp_gr = gmax > comp_gr * 0.97f ? gmax : comp_gr * 0.97f;
        } else
            comp_gr = 0.0f;
        {                                /* LOAF BREAKER: the last guard, a look-ahead limiter: the mix delayed 64 samples
                                          * (1.5 ms) while its level, seen ahead, sets the gain, which comes down smoothly
                                          * before a peak reaches the output (no sudden steps: they were the distortion
                                          * heard with many sounds at once) and goes back up over ~150 ms (slow enough
                                          * not to wobble the bass); what still passes 0.95 rounded off softly under 1 */
            static float ll[LIM_LA], lr[LIM_LA], lenv, lg = 1.0f;
            static uint32_t lw;
            for (i = 0; i < m; i++) {
                float a = brk_absf(l[i]) > brk_absf(r[i]) ? brk_absf(l[i]) : brk_absf(r[i]), tgt, dl, dr;
                if (a > lenv)
                    lenv = a;            /* (seen ahead: at once) */
                else
                    lenv += (a - lenv) * 0.00015f;   /* (~150 ms back) */
                tgt = lenv > 0.95f ? 0.95f / lenv : 1.0f;
                lg += (tgt - lg) * (tgt < lg ? 0.078f : 0.0003f);   /* (down within the look-ahead, up slowly) */
                dl = ll[lw];
                dr = lr[lw];
                ll[lw] = l[i];
                lr[lw] = r[i];
                lw = (lw + 1u) % LIM_LA;
                l[i] = lim_soft(dl * lg);
                r[i] = lim_soft(dr * lg);
            }
        }
        if (scope_on) {                  /* LOAF BREAKER: the CDJ view's columns (scope), from what leaves */
            static float l1, l2, pk[4];
            static uint32_t sn;
            uint32_t len = 441000u / (set.tempo_x10 >= 400u ? set.tempo_x10 : 1700u), k;
            for (i = 0; i < m; i++) {
                float x = 0.5f * (l[i] + r[i]), v[4];
                l1 += (x - l1) * 0.0285f;        /* (~200 Hz) */
                l2 += (x - l2) * 0.356f;         /* (~2.5 kHz) */
                v[0] = brk_absf(l1);
                v[1] = brk_absf(l2 - l1);
                v[2] = brk_absf(x - l2);
                v[3] = brk_absf(l[i]) > brk_absf(r[i]) ? brk_absf(l[i]) : brk_absf(r[i]);
                for (k = 0; k < 4u; k++)
                    if (v[k] > pk[k])
                        pk[k] = v[k];
                if (++sn >= len) {
                    static const float G[4] = {300.0f, 380.0f, 520.0f, 255.0f};   /* (the bands as heard: the highs
                                          * up, they are quieter) */
                    uint8_t *c = scope[scope_w % SCOPE_N];
                    for (k = 0; k < 4u; k++) {
                        c[k] = (uint8_t)brk_clampf(pk[k] * G[k], 0.0f, 255.0f);
                        pk[k] = 0.0f;
                    }
                    sn = 0u;
                    scope_w++;
                }
            }
        }
        {                                /* the header's meter: as it leaves, after MASTER */
            float out = gain * (1.0f / 8388608.0f), a = meter(l, m, 0.0f) * out, b = meter(r, m, 0.0f) * out;
            meter_l = a > meter_l * 0.987f ? a : meter_l * 0.987f;
            meter_r = b > meter_r * 0.987f ? b : meter_r * 0.987f;
            if (a > 0.001f || b > 0.001f)
                out_sound = 1u;
        }
        for (i = 0; i < m; i++) {
            lr[2u * i] = (int32_t)(l[i] * gain);
            lr[2u * i + 1u] = (int32_t)(r[i] * gain);
        }
        lr += 2u * m;
        n -= m;
    }
}

/* (the audio interrupt, right after a render that took over 90 % of its time: PRD E-7) */
void app_overload(void) { brk_fx_overload(); }

void app_panic(void)
{
    muted = 1u;
    brk_play(0);
}

/* ----------------------------------------------------------------------- UI --- */
#include "ui/common.c"
#include "ui/fx.c"
#include "ui/pads.c"
#include "ui/keys.c"
#include "ui/bta.c"
#include "ui/global.c"
#include "ui/ovl.c"
#include "ui/seq.c"
#include "ui/kick.c"
#include "ui/perc.c"
#include "ui/mixer.c"
#include "ui/track.c"                 /* (LOAF BREAKER: ENV's TRACKS, PLAY, DROP) */
#include "ui/tutor.c"                 /* (LOAF BREAKER: the TUTORIAL) */
#include "ui/screen.c"
#if CH_DEMO
#include "ui/demo.c"
#else                                    /* GLOUPI BREAKS: no demo (ui/demo.c left out): what main.c calls of it, empty */
static const struct { uint8_t on; } demo = {0u};
static void demo_start(uint32_t now) { (void)now; }
static void demo_end(void) {}
static void demo_frame(uint32_t now) { (void)now; }
static void demo_leds(uint32_t now) { (void)now; }
#endif
#include "ui/cdj.c"                   /* (LOAF BREAKER: HOME held: the break scrolling, as on a CDJ) */
#include "ui/main.c"
#undef C_WHITE                           /* (gfx.c's white again after the UI: skins, common.c) */
#define C_WHITE 0xFFFFu
