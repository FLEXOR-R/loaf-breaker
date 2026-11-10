/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the app's half of the contract with fm1-base (device/unity.c, host/plat_host.c) and its settings. */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "brk.h"
#include "brk_fx.h"

#define APP_NAME "LOAF BREAKER"     /* LOAF BREAKER (was GLOUPI BREAKS): the name on the platform's screens */
#define APP_TAGLINE "AMEN CHOPPER"
#define APP_SPLASH_RGB RGB(255, 132, 112)   /* GLOUPI BREAKS red (gfx.c RED's brightest) */
#define APP_SYS_TEXT                     /* the platform's own screens (crash, safe mode, update, calibration) write in
                                          * CHOPPAH's faces (app_sys_* in app.c): fm1-base's own 15- and 31-pixel ones
                                          * stay out of the flash (28 KB) */
#define APP_SPLASH                       /* the first screen at power-on is the app's (app_splash) */
void app_splash(void);
#define CH_RESET_TURNS 15u              /* GLOUPI BREAKS: RESET ALL wants K4 turned 15 times (CHOPPAH: 2) */
#define CH_RESET_TEXT "15"
#define CH_TAGLINE 0                     /* GLOUPI BREAKS: no "AMEN CHOPPER  FOR THE FM-1" under the name */
#define CH_LOGO 1                        /* LOAF BREAKER: its picture on the splash, 1.5 s (splash_data.h), its
                                          * lettering on the start screen (title_data.h) */
#define CH_DEMO 0                        /* GLOUPI BREAKS: no demo (CHOPPAH: 1, ui/demo.c) */
#define APP_PALETTE 3u                   /* gfx.c's red (GLOUPI BREAKS; CHOPPAH: 1u, amber) */
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_CANVAS_PX (240u * 60u)       /* draw in strips of 60 rows at most: the pool goes to the breaks (and the
                                          * Bluetooth audio queue) */
#if defined(__has_include)               /* LOAF BREAKER: the release's own number (gloupi.yml writes it: 0.100 ...,
                                          * whatever the package's) */
#if __has_include("loaf_version.h")
#include "loaf_version.h"
#endif
#endif
#ifdef LOAF_VERSION
#define CH_VERSION LOAF_VERSION
#elif defined(APP_VERSION)              /* GLOUPI BREAKS: the release's number (gloupi.yml: 0.02, 0.03 ...) */
#define CH_VERSION APP_VERSION
#else
#define CH_VERSION "DEV"
#endif

/* MIDI and Bluetooth (PRD N-1, N-2, N-5): the GLOBAL page's clock in / out and Bluetooth MIDI. The platform calls
 * app_loop every pass of the main loop (MIDI in), app_tick at 1 kHz (clock out on time); Bluetooth starts at
 * power-on as the saved setting says, advertising as "CHOPPAH XXXX" */
#define APP_LOOP
#define APP_OVERLOAD                     /* the render over 90 % of its time: the FX chain sheds slots (PRD E-7) */
void app_loop(void);
#define APP_TICK
void app_tick(void);
int app_ble_at_boot(void);
#define APP_BLE_AT_BOOT (app_ble_at_boot())
#define BLE_DEVICE_NAME "CHOPPAH"
/* Bluetooth audio out (fm1.toml bt_audio, PRD A-72): its packet queue (40 x 332 B) in the pool, not RAM (the RAM is
 * the tighter here: the reverb's lines are in it) */
#define BP_MQ_ATTR PLAT_POOL

/* the store (PRD 8.4): settings in Felucca's settings pair, the panel calibration where every fm1-base firmware
 * keeps it. The break bank will take 0x97000..0xD9FFF and PLAT_XDATA1 (M5), patterns 0xDA000 (M2). */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_BLE, OBJ_PAT, OBJ_STEP, OBJ_PG0, OBJ_KG0 = OBJ_PG0 + 4, OBJ_SG0 = OBJ_KG0 + 4,
       OBJ_NG0 = OBJ_SG0 + 4, OBJ_KLAB = OBJ_NG0 + 4, OBJ_NOBJ };   /* (LOAF BREAKER: OBJ_STEP, each drum step's
                                                                 * HITS, CHANCE, NUDGE: 0xD8000, the old bank's room) */   /* OBJ_BLE: Bluetooth's address and pairing;
                                                                 * OBJ_PAT: the 16 patterns */
/* LOAF BREAKER: 64 patterns, four groups of 16, each group's own three objects (OBJ_PG0 + g its patterns, OBJ_KG0 + g
 * their kick notes and HITS, OBJ_SG0 + g their drum steps), 0xC0000..0xD7FFF (the old bank's room); OBJ_NG0 + g their
 * hoover notes past -8..+7 (HV KEYS), 0xB8000..0xBFFFF; OBJ_KLAB the KICK LAB (each kick's TAIL and STACK), 0xB6000 */
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xDE000u, 0xDA000u, 0xD8000u, \
                       0xC0000u, 0xC2000u, 0xC4000u, 0xC6000u, 0xC8000u, 0xCA000u, 0xCC000u, 0xCE000u, \
                       0xD0000u, 0xD2000u, 0xD4000u, 0xD6000u, 0xB8000u, 0xBA000u, 0xBC000u, 0xBE000u, 0xB6000u}

/* the pool: the break playing, decoded to 16-bit PCM (PRD 8.2). M1 has one buffer; M5 adds the second */
#define CH_PCM_MAX 106496u               /* samples: 4.8 s at 22.05 kHz (2 bars at 99.4 BPM and up: Funky Drummer) */
#define CH_RATE 22050u                   /* the built-in break's rate */

/* the contract: the platform calls these */
void app_init(void);                     /* boot: the engine, then the saved settings (or defaults) */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);   /* audio interrupt: n stereo frames, 24-bit */
void app_panic(void);                    /* silence now (before an update rewrites the flash) */
void app_ui_init(void);                  /* the first screen, after the splash */
void app_frame(void);                    /* the UI: input and a redraw, about 60 times a second */
void app_input_only(void);               /* between frames: input only */
void app_say(const char *a, const char *b);   /* a short message on the screen */

/* the host simulator's own commands (app_host_cmd in app.c) */
#define APP_HOST_CMD
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
#define APP_HOST_FAILS                   /* expect_bpm's failures, counted into the run's */
int app_host_fails(void);
#define APP_HOST_EXPECT                  /* expect page / gpage / kpage / seq_lane / fx_focus / fx_on / lane_kick /
                                          * lane_hat / pattern / brk / brk_next / help / drums_chain / lobo */
int app_expect(const char *what, int *got);
