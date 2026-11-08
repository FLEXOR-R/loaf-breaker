/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER TEST: the OVERLAY page gone (ARP is FILL): what stays, the ARP black key's effect. CHOPPAH: the OVERLAY page (ARP, PRD B-52). Each white key an effect over everything that plays (dsp/ovl.c), on
 * while held, any together; with LATCH a press switches its effect on and off (and latched ones stay on on the other
 * pages). K1 the amount of the effect touched last, K2 the rate of the ones on the beat, K3 the sweeps' length,
 * K4 LATCH. (Part of app.c's unity: included there in order.) */


static uint32_t ovl_pad_bits(void) { return ovl_pad_on() ? 1u << (set.ovl_pad & 15u) : 0u; }   /* the ARP key's */
static void ovl_push(void)
{
    ovl_set(ovl_pad_bits(), set.ovl_amt, set.ovl_rate, set.ovl_sweep);
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

