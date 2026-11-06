/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: the update loader's screen (app/loader/ldr_screen.c) drawn on a computer, through a stand-in panel
 * (tests/loader/boards/fm1/fm1_lcd_hw.h), at 0, half and all written: ldr_0.ppm, ldr_50.ppm, ldr_100.ppm here.
 *   cc -std=c99 -I tests/loader/src tests/loader/src/preview.c -o ldr_preview && ./ldr_preview */
#include <stdio.h>
#include "../../../app/loader/ldr_screen.c"

static int dump(const char *name)
{
    FILE *f = fopen(name, "wb");
    uint32_t i;
    if (!f)
        return 1;
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = lcd_fb[i];
        fputc((c >> 11) * 255 / 31, f);
        fputc(((c >> 5) & 63) * 255 / 63, f);
        fputc((c & 31) * 255 / 31, f);
    }
    return fclose(f) != 0;
}

int main(void)
{
    int bad = 0;
    ldr_screen_init();
    bad |= dump("ldr_0.ppm");
    ldr_screen_progress(1u, 2u);
    bad |= dump("ldr_50.ppm");
    ldr_screen_progress(2u, 2u);
    bad |= dump("ldr_100.ppm");
    return bad;
}
