/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-loop bridge from physical or simulated key readings to the live player.
 * The matrix, ADC and calibration live in fm1_input.h and cr_sense.c. */
static uint32_t cf_pressure_keys;   /* physical presses actually accepted by the chord player */
#ifndef CR_SENSE_KEYS
static uint8_t cf_sim_velocity[CF_NKEYS], cf_sim_pressure[CF_NKEYS];
#endif

static uint8_t cf_key_strike(unsigned key)
{
    if (!(cs.expression & CF_EXPR_VEL))
        return 0;
#ifdef CR_SENSE_KEYS
    return (uint8_t)cr_sense_take(key);
#else
    return cf_sim_velocity[key];
#endif
}

static int cf_has_selector(uint32_t keys)
{
    while (keys) {
        unsigned key = __builtin_ctz(keys);
        int idx;
        if (cf_key_role(cs.chord.algo, key, &idx) == CF_ROLE_BUTTON)
            return 1;
        keys &= keys - 1u;
    }
    return 0;
}

static int cf_wait_for_strike(unsigned key)
{
    int idx;
    if (cf_key_role(cs.chord.algo, key, &idx) != CF_ROLE_ROOT ||
        (cu.scr == SCR_EDIT && cf_key_is_upper(key)) ||
        (cu.scr == SCR_SQ && (squ.pat == SQ_SONGPG || sq_frame_trk(SQ_DRUMS))) ||
        (cu.scr == SCR_SQED && squ.trk == SQ_DRUMS))
        return 0;
#ifdef CR_SENSE_KEYS
    if (cr_sense_land[key])
        return 0;
#else
    if (cf_sim_velocity[key])
        return 0;
#endif
    return fm1_ticks() - CF_KEY_T0(key) < 20000u * FM1_TICKS_PER_US;
}

static void cf_update_pressure(uint32_t now, uint32_t released)
{
    static uint32_t pressure_ms;
#ifdef CR_SENSE_KEYS
    while (released) {
        unsigned key = __builtin_ctz(released);
        cr_sense_land[key] = 0;
        sn_p_on[key] = sn_p_s[key] = cr_sense_at[key] = 0;
        released &= released - 1u;
    }
#endif
    cf_pressure_keys &= now;
    if (!(cs.expression & CF_EXPR_AT) || !cf_pressure_keys) {
        cf_channel_pressure = 0;
    } else if (fm1_ms - pressure_ms >= 5u) {
        uint32_t held = cf_pressure_keys & 0xfffffu;
        uint8_t peak = 0;
        pressure_ms = fm1_ms;
#ifdef CR_SENSE_KEYS
        cr_sense_pressure_poll(fm1_ms, 0);
#endif
        while (held) {
            unsigned key = __builtin_ctz(held);
#ifdef CR_SENSE_KEYS
            uint8_t pressure = cr_sense_at[key];
#else
            uint8_t pressure = cf_sim_pressure[key];
#endif
            if (pressure > peak)
                peak = pressure;
            held &= held - 1u;
        }
        cf_channel_pressure = peak;
    }
}
