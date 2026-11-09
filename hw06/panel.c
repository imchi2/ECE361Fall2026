/**
 * @file panel.c
 * @brief Homework 6, Part 2: switch edges and LED writes.
 *
 * The two edge functions are pure bit arithmetic on two snapshots of
 * the switch register - no registers, no state. The two LED functions
 * are read-modify-write on LEDS_REG: read the whole word, change one
 * bit, write the word back, so every other LED keeps its state.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */

#include "panel.h"
#include "iom361_r4.h"

uint32_t panel_pressed(uint32_t now, uint32_t before)
{
    /* A switch counts as pressed when it is on now and was off
       before: keep the bits that are set in now and clear in before.
       A switch that was already on stays out of the answer, so a held
       switch is reported once, on the tick it went down. */
    return now & ~before;
}

uint32_t panel_released(uint32_t now, uint32_t before)
{
    /* The mirror image: on before, off now. */
    return before & ~now;
}

int panel_led(uint32_t *base, int n, int on)
{
    int rc;
    uint32_t leds;
    uint32_t mask;

    /* Reject a bad LED number before touching the register - the
       same rule as hw01: validation first, side effects after. */
    if (n < 0 || n > 31)
        return -1;

    /* Bit n as a mask. n is 0..31 here, so the shift is always
       defined (shifting by 32 or more would not be). */
    mask = (uint32_t) 1 << n;

    /* Read-modify-write: read the whole register, change only bit n,
       write the word back. Writing a freshly built word instead would
       switch every other LED off - which is exactly the mistake the
       Shine question "why read before it writes?" is about. */
    rc = 0;
    leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0)
        return rc;

    if (on != 0)
        leds = leds | mask;
    else
        leds = leds & ~mask;

    rc = 0;
    iom361_writeReg(base, LEDS_REG, leds, &rc);

    return rc;
}

int panel_toggle(uint32_t *base, int n)
{
    int rc;
    uint32_t leds;
    uint32_t mask;

    /* Same validation as panel_led, repeated here on purpose: each
       function checks its own arguments before it does anything. */
    if (n < 0 || n > 31)
        return -1;

    mask = (uint32_t) 1 << n;

    /* Same read-modify-write, with XOR instead of set/clear: bit n
       flips, every other bit comes back exactly as it was. */
    rc = 0;
    leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0)
        return rc;

    leds = leds ^ mask;

    rc = 0;
    iom361_writeReg(base, LEDS_REG, leds, &rc);

    return rc;
}
