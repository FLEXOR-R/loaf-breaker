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
#include "egg_data.h"                /* GLOUPI BREAKS: LOAF LORD (tools/make_egg.py) */
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
    uint8_t half_mt_off;                 /* GLOUPI BREAKS: HALF SPEED's K2, 1 MASTER TEMPO off (0, older saves: on) */
    uint8_t ovl_pad;                     /* GLOUPI BREAKS: the ARP black key, bits 0-3 its OVERLAY effect (OV_*),
                                          * bit 4 its LATCH (gone), bit 5 OFF (K3: the effects browsed silent),
                                          * bits 6-7 THRU (K2): 0 BOTH, 1 BREAK (the kick around the chain and
                                          * the overlays), 2 KICK (the overlays on the kick alone) */
    /* (appended: LOAF BREAKER's MORE page; 0, older saves: off) */
    uint8_t auto_save;                   /* K2 AUTO SAVE: 1, a minute after a change, saved once stopped */
    uint8_t vinyl;                       /* the VINYL BREAK key's THRU (K3): 0 BOTH (older saves too), 1 BREAK, 2 KICK
                                          * (the kick, the snare and the hat) */
    uint8_t kslot;                       /* the kick memory playing, 0..9 (KICK 1..10): set.kick is it */
    uint8_t duck;                        /* LOAF BREAKER (crossbreed): DUCK, the break down under each kick, 0 off .. 100
                                          * (SEL + K1) */
    /* (appended: LOAF BREAKER's kick memories; an older save's: the built-in kicks, its own kick KICK 1; one of six
     * kept them, KICK 7..10 begin anew) */
#define KICK_MEMS 10u
    kick_t kmem[KICK_MEMS];              /* KICK 1..10, each as left (PRESETS on KICK, SELECT on SEQ: nothing lost) */
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
    uint8_t limit;                       /* SEL + K3: the mix driven into a limiter, 0 off .. 100 (+18 dB in) */
    uint8_t rsv6[3];
    /* (appended: LOAF BREAKER's KICK EQ; an older save's: flat) */
    float keq[8];                        /* HOME's third part: LOW, MID, HIGH, FREQ, LO FREQ, MID Q, HI FREQ, LEVEL */
    /* (appended: LOAF BREAKER's RAVE STAB; an older save's: its first preset) */
    perc_voice_t pstab;                  /* its sound (PV_STAB) */
    uint8_t sslot;                       /* its memory playing, 0..9 */
    uint8_t rsv8[3];
    perc_voice_t smem[PERC_MEMS];        /* its memories */
    /* (appended: LOAF BREAKER's DROP and TRACK, ENV's parts; an older save's: a 4-bar build into the next pattern) */
    uint8_t drop_bars;                   /* the build: DROP_BARS's index (0: none, the drop at the bar's end) */
    uint8_t drop_after;                  /* the pattern the drop brings in: 0 the next one, 1.. that one */
    uint8_t drop_crash;                  /* the drop's crash, 0 off .. 100 */
    uint8_t trk;                         /* the track TRACKS edits and PLAY plays, 0..NTRACKS-1 */
} settings_t;
#define SET_SIZE_MIN offsetof(settings_t, rsv2)   /* the shortest save that loads: CHP7's first size */
_Static_assert(sizeof(settings_t) <= 3840u, "a store object holds 3840 bytes");
static settings_t set;
static perc_voice_t *perc_voice(uint32_t v)   /* LOAF BREAKER: a drum's sound, PV_* (the snare and the hat in perc,
                                          * the rest in pmore) */
{
    v %= PERC_VOICES;
    return v < 2u ? &set.perc.v[v] : v == PV_CLAP ? &set.pmore[0] : &set.pstab;
}
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

static void settings_defaults(void)
{
    uint32_t k;
    uint8_t *z = (uint8_t *)&set;
    for (k = 0; k < sizeof set; k++)     /* (every field: no leftovers from a save that didn't load) */
        z[k] = 0u;
    set.magic = SET_MAGIC;
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
    set.drums_chain = 1u;
    brk_padset_defaults(&set.pads);
    for (k = 0; k < BRK_KEYS; k++) {
        brk_keyset_defaults(&set.keys[k]);
        brk_slicefx_defaults(&set.slx[k]);
    }
    kick_defaults(&set.kick);
    for (k = 0; k < KICK_MEMS; k++) {    /* LOAF BREAKER: the kick memories begin as the built-in kicks (7..10: the
                                          * first four again) */
        kick_preset(&set.kmem[k], k % KICK_PRESETS);
        set.kmem[k].volume = set.kick.volume;
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
        set.pstab = d.v[PV_STAB];
        for (k = 0; k < PERC_VOICES * PERC_MEMS; k++)   /* (the memories begin as the presets, again and again) */
            perc_preset(perc_mem(k / PERC_MEMS, k % PERC_MEMS), k / PERC_MEMS, k % PERC_MEMS % PERC_PRESETS);
    }
    for (k = 0; k < 16u; k++)            /* the overlays: full, the sweeps and the dirt a little less */
        set.ovl_amt[k] = k == OV_DIVE || k == OV_RISE || k == OV_CRUSH || k == OV_DIST || k == OV_BOOM ? 70u : 100u;
    set.ovl_rate = 2u;                   /* 1/16 */
    set.ovl_sweep = 2u;                  /* a bar */
    for (k = 0; k < BRK_FX_SLOTS; k++) {
        brk_mbc_defaults(&set.mbc[k]);
        brk_fxx_defaults(&set.fxx[k], set.fx.slot[k].type);
    }
    set.drop_bars = 3u;                  /* (LOAF BREAKER: DROP: a 4-bar build, into the next pattern, the crash full) */
    set.drop_crash = 100u;
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
static void settings_apply(void)
{
    brk_tempo((float)set.tempo_x10 * 0.1f);
    brk_pitch((float)set.pitch);
    brk_swing((float)set.swing * 0.01f);
    brk_gen(&set.gen);
    brk_fx_set(&set.fx);
    {
        uint32_t k;
        for (k = 0; k < BRK_FX_SLOTS; k++) {
            brk_fx_set_mbc(k, &set.mbc[k]);
            brk_fx_set_fxx(k, &set.fxx[k]);
        }
    }
    brk_fx_tempo((float)set.tempo_x10 * 0.1f);
    set.pads.roll_pitch = 0.0f;          /* LOAF BREAKER: the rolls' pitch gone (an older save's: off) */
    brk_padset(&set.pads);
    brk_pitch_mode(pitch_keeps(), (float)(set.grain_ms ? set.grain_ms : 30u));
    brk_half_master(!set.half_mt_off);
    brk_vinyl((set.vinyl & 3u) != 2u);
    ovl_reese_hz(kick_pitch_hz(KL_SUB, set.kick.l[KL_SUB].pitch));
    brk_fx_set_kick_eq(set.keq);   /* (the ARP's REESE in the kick's key) */
    brk_gate((float)(set.gate ? set.gate : 100u) * 0.01f);
    brk_snap(SNAP_SL[snap_index()]);
    kick_set(&set.kick);
    {
        perc_t pa;                       /* (all the drums' voices for the engine) */
        uint32_t k;
        for (k = 0; k < PERC_VOICES; k++)
            pa.v[k] = *perc_voice(k);
        pa.volume = set.perc.volume;
        perc_set(&pa);
    }
    skin_apply(set.skin);
    {
        uint32_t k;
        for (k = 0; k < BRK_KEYS; k++) {
            brk_keyset(k, &set.keys[k]);
            brk_slicefx(k, &set.slx[k]);
        }
    }
}

static void changed(void)
{
    set_dirty = 1u;
    foot_dirty = 1u;
    settings_apply();
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
enum { DR_KICK, DR_SNARE, DR_HAT, DR_CLAP, DR_STAB, NDRUMS };
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
} pats_t;
static pats_t pats;
#define PATS_SIZE_OLD offsetof(pats_t, trk)   /* (a save before the tracks) */
#define PATS_SIZE_TRK offsetof(pats_t, mem)   /* (one before the effects' memory) */
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
static volatile uint8_t build_n;         /* three clicks: a build, or (building) the drop */
static volatile uint16_t drum_next[NDRUMS][4];   /* the next pattern's drums, for the drop */
static volatile uint8_t drop_to = 0xFFu; /* the pattern the drop brings in (its break's steps, at the bar before) */
static volatile uint8_t drum_next_ok;
static volatile uint8_t drop_n;
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
    }
    trk_defaults();
    pats.mem.fx_ok = pats.mem.kfx_ok = 0u;
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
static const uint8_t LEN_BIT[NDRUMS] = {0u, PAT_SNARE32, 8u, 16u, 32u};   /* (the stab: bit 5) */
/* LOAF BREAKER: the rave stab, in each step's flags byte (st[k][2], its bits 0-2 the break's): bit 7 its hit, bits
 * 3-6 its note (-8 .. +7 semitones), for each of the 32 steps (its two bars) */
static int32_t stab_note(const pat_t *p, uint32_t k)
{
    uint32_t v = (uint32_t)p->st[k % PAT_STEPS][2] >> 3 & 15u;
    return v >= 8u ? (int32_t)v - 16 : (int32_t)v;
}
static void stab_note_set(pat_t *p, uint32_t k, int32_t v)
{
    uint8_t *b = &p->st[k % PAT_STEPS][2];
    *b = (uint8_t)((*b & 0x87u) | ((uint32_t)(v < -8 ? -8 : v > 7 ? 7 : v) & 15u) << 3);
}
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
    if (d == DR_STAB) {                  /* (its hits: bit 7 of the bar's steps) */
        uint32_t m = 0u, s;
        for (s = 0; s < 16u; s++)
            m |= (uint32_t)(p->st[(b & 1u) * 16u + s][2] >> 7) << s;
        return m;
    }
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
    else if (d == DR_STAB) {
        uint32_t s;
        for (s = 0; s < 16u; s++) {
            uint8_t *f = &p->st[(b & 1u) * 16u + s][2];
            *f = (uint8_t)((*f & 0x7Fu) | (m >> s & 1u) << 7);
        }
    } else {
        k = drum_at(d, b);
        p->st[k][3] = (uint8_t)m;
        p->st[k + 1u][3] = (uint8_t)(m >> 8);
    }
}
/* LOAF BREAKER (crossbreed): the kick's notes, one a step of the bar (every bar alike), -8 .. +7 semitones, four bits
 * each in the spare bytes of steps 13-20 (0: as the kick is) */
static int32_t kick_note(const pat_t *p, uint32_t s)
{
    uint32_t b = p->st[13u + (s & 15u) / 2u][3], v = s & 1u ? b >> 4 : b & 15u;
    return v >= 8u ? (int32_t)v - 16 : (int32_t)v;
}
static void kick_note_set(pat_t *p, uint32_t s, int32_t v)
{
    uint8_t *b = &p->st[13u + (s & 15u) / 2u][3];
    uint32_t n = (uint32_t)(v < -8 ? -8 : v > 7 ? 7 : v) & 15u;
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

void app_init(void)
{
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
    pats.mem.fx_ok = pats.mem.kfx_ok = 0u;   /* (an older save: no effect remembered) */
    {
        int sz = plat_store_size(OBJ_PAT);   /* (LOAF BREAKER: a save from before the tracks: its patterns) */
        if ((sz != (int)sizeof pats && sz != (int)PATS_SIZE_OLD && sz != (int)PATS_SIZE_TRK) ||
            plat_store_load(OBJ_PAT, &pats, (uint32_t)sz) != sz)
            pats.magic = 0u;
    }
    if (pats.magic != PAT_MAGIC && pats.magic != PAT2_MAGIC && pats.magic != PAT1_MAGIC)
        pat_defaults();
    else if (pats.magic != PAT_MAGIC) {  /* an older save: its kick again over four bars, as it played */
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
            else if (on && (note == 36u || (note >= 38u && note <= 40u) || note == 42u || note == 44u || note == 46u))
                midi_drum(note == 36u ? DR_KICK : note == 38u ? DR_SNARE : note == 39u ? DR_CLAP : note == 40u ? DR_STAB
                          : DR_HAT);     /* (LOAF BREAKER: 39 the clap, 40 the stab) */   /* (LOAF
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
    static float mono[APP_BLOCK], l[APP_BLOCK], r[APP_BLOCK], echo[APP_BLOCK], verb[APP_BLOCK];
    static float kick[APP_BLOCK], perc[APP_BLOCK];
    static float kl[APP_BLOCK], kr[APP_BLOCK];   /* LOAF BREAKER: the kick through the overlays alone (THRU KICK) */
    static float spd[APP_BLOCK], tape0 = 1.0f;   /* LOAF BREAKER: the VINYL BREAK's speed on the drums, a sample each */
    static uint8_t b_on, b_drop, b_rew;  /* LOAF BREAKER: building; the drop asked; the drop armed for the next
                                          * step (the engine rewound to the pattern's first: brk_rewind) */
    static uint32_t b_steps;             /* steps built */
    static int32_t half_due = -1;        /* the roll's 32nd between two steps: samples from this block's start */
    static float crash;                  /* the drop's crash, its level */
    int vin = (set.vinyl & 3u) != 1u;    /* (THRU BOTH or KICK: the drums slow too) */
    uint32_t thru = (set.ovl_pad >> 6) & 3u;   /* the ARP key's THRU: 0 BOTH, 1 BREAK, 2 KICK */
    int kick_dry = thru != 0u;           /* the kick apart from the chain (BREAK: around the overlays too) */
    float gain = muted ? 0.0f : (float)master_q12 * (1.0f / 4096.0f) * 8300000.0f;
    brk_status_t st;
    uint32_t t0 = plat_ms(), done = 0u, total = n;
    if (sync_c != sync_seen) {           /* the outside clock's beat, and how long ago it came */
        float age = (float)(t0 - sync_ms) / MS_PER_SAMPLE;
        sync_seen = sync_c;
        if (age < (float)BRK_SR * 0.25f)
            brk_sync(sync_n16, age);
    }
    while (n) {
        uint32_t m = n < APP_BLOCK ? n : APP_BLOCK, i;
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
            float spm[8];                /* (each stab's note, a factor, by its hit) */
            uint8_t idx[8], who[8];
            uint32_t k, ne = brk_step_events(off, idx, 8u), nk = 0u, np = 0u;
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
                b_on = b_drop = b_rew = build_wait = 0u;   /* (stopped: no build) */
            if (b_on && half_due >= 0 && half_due < (int32_t)m) {   /* (the 32nd between two steps) */
                pa[np] = (uint16_t)half_due;
                who[np++] = 1u << PV_SNARE;
            }
            if (half_due >= 0 && (half_due -= (int32_t)m) < 0)
                half_due = -1;
            for (k = 0; k < ne; k++) {
                uint32_t s = idx[k] & 15u;   /* the step's place in the bar (idx: in four bars, the kick's) */
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
                        if (!(b_steps % div) && np < 8u) {   /* (quarters, eighths, 16ths, 32nds: a quarter each) */
                            pa[np] = off[k];
                            who[np++] = 1u << PV_SNARE;
                            if (q4 >= 3u)
                                half_due = (int32_t)off[k] + (int32_t)((float)BRK_SR * 7.5f / brk_get_tempo());
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
                uint32_t w = 0u, d, bar = idx[k] >> 4 & 3u;
                for (d = DR_SNARE; d < NDRUMS; d++)   /* (LOAF BREAKER: every drum, its bar) */
                    w |= (uint32_t)(drum_now[d][bar] >> s & 1u) << (d - 1u);
                if (drum_now[DR_KICK][bar] >> s & 1u) {
                    int32_t kn = kick_note(&pats.p[set.pattern % NPAT], s);
                    kpm[nk] = kn ? brk_exp2f((float)kn * (1.0f / 12.0f)) : 1.0f;
                    ka[nk++] = off[k];
                }
                if (w >> PV_STAB & 1u) {     /* the stab: its step's note (its bars: its length) */
                    int32_t sn = stab_note(&pats.p[set.pattern % NPAT],
                                           (bar % drum_bars(&pats.p[set.pattern % NPAT], DR_STAB)) * 16u + s);
                    spm[np] = sn ? brk_exp2f((float)sn * (1.0f / 12.0f)) : 1.0f;
                }
                if (w) {
                    pa[np] = off[k];
                    who[np++] = (uint8_t)w;
                }
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
            kick_pitches(kpm);
            kick_render(kick, m, ka, nk);
            brk_fx_kick_eq(kick, m);    /* LOAF BREAKER: HOME's KICK EQ */
            perc_pitches(spm);
            perc_render(perc, m, pa, who, np);
            if (set.duck) {              /* LOAF BREAKER (crossbreed): DUCK: the break (and its throws) pushed down at
                                          * each kick, in ~0.3 ms, back up over ~150 ms */
                static float dt, de;
                float depth = (float)(set.duck > 100u ? 100u : set.duck) * 0.01f;
                uint32_t j = 0u;
                for (i = 0; i < m; i++) {
                    float g;
                    while (j < nk && ka[j] <= i) {
                        dt = 1.0f;
                        j++;
                    }
                    de += (dt - de) * 0.08f;
                    dt *= 0.9997f;
                    g = 1.0f - depth * de;
                    mono[i] *= g;
                    echo[i] *= g;
                    verb[i] *= g;
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
            int bu = b_on && build_len;
            float p = bu ? build_p : 0.0f, rz = 0.22f * p * p;
            float kk = brk_clampf(6.2832f * 30.0f * brk_exp2f(p * 6.5f) / (float)BRK_SR, 0.0f, 0.9f);
            for (i = 0; i < m; i++) {
                float n;
                nz = nz * 1664525u + 1013904223u;
                n = (float)(int32_t)nz * (1.0f / 2147483648.0f);
                if (bu) {                /* (30 Hz up to ~2.7 kHz, two poles) */
                    float xl = l[i] + n * rz, xr = r[i] + n * rz;
                    h1l += (xl - h1l) * kk;
                    xl -= h1l;
                    h2l += (xl - h2l) * kk;
                    l[i] = xl - h2l;
                    h1r += (xr - h1r) * kk;
                    xr -= h1r;
                    h2r += (xr - h2r) * kk;
                    r[i] = xr - h2r;
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
        if (set.limit) {                 /* LOAF BREAKER: LIMIT: the whole mix driven in (up to +18 dB) and held under
                                          * its ceiling: louder, never over (the peak followed at once, let go over
                                          * ~75 ms) */
            static float lenv;
            float gin = brk_exp2f((float)(set.limit > 100u ? 100u : set.limit) * 0.03f);
            for (i = 0; i < m; i++) {
                float xl = l[i] * gin, xr = r[i] * gin, a = brk_absf(xl) > brk_absf(xr) ? brk_absf(xl) : brk_absf(xr);
                float gr;
                if (a > lenv)
                    lenv = a;
                else
                    lenv += (a - lenv) * 0.0003f;
                gr = lenv > 0.9f ? 0.9f / lenv : 1.0f;
                l[i] = brk_clampf(xl * gr, -0.95f, 0.95f);
                r[i] = brk_clampf(xr * gr, -0.95f, 0.95f);
            }
        }
        {                                /* the header's meter: as it leaves, after MASTER */
            float out = gain * (1.0f / 8388608.0f), a = meter(l, m, 0.0f) * out, b = meter(r, m, 0.0f) * out;
            meter_l = a > meter_l * 0.987f ? a : meter_l * 0.987f;
            meter_r = b > meter_r * 0.987f ? b : meter_r * 0.987f;
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
#include "ui/main.c"
#undef C_WHITE                           /* (gfx.c's white again after the UI: skins, common.c) */
#define C_WHITE 0xFFFFu
