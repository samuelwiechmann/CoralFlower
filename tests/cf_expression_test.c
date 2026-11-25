/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the same key, audio, and USB queues used by the browser. */
#include "../tools/emu/web/emu_web.c"
#include <assert.h>

static unsigned expect_strikes(unsigned velocity)
{
    unsigned count = 0, packet;
    while ((packet = web_midi_out_take())) {
        if (((packet >> 12) & 15) == 9) {
            assert((packet >> 24) == velocity);
            count++;
        }
    }
    return count;
}

static void keys(unsigned mask, unsigned ms)
{
    web_keys(mask);
    web_step(ms);
}

static void setting(unsigned flags)
{
    cs.expression = CF_EXPR_DEFAULT | flags;
    cf_send_config();
    web_step(40);
}

int main(void)
{
    const unsigned root = 7, major = 20;
    web_boot();
    web_step(1900);
    /* GLO's last page and its three encoders are the normal settings route. */
    web_buttons(1u << EMU_B_GLO);
    web_step(40);
    web_buttons(0);
    web_step(40);
    web_enc(EMU_E_SELECT, -1);
    web_step(40);
    assert(cu.scr == SCR_GLO && cu.page[SCR_GLO] == 6);
    web_enc(EMU_E_K1, 1);
    web_enc(EMU_E_K2, 2);
    web_enc(EMU_E_K3, 1);
    web_step(900);
    uint8_t saved = cs.expression;
    assert(saved == (CF_EXPR_TAG | (1u << 2) | CF_EXPR_VEL | CF_EXPR_AT));
    cs.expression = 0;
    cf_settings_load();
    assert(cs.expression == saved);
    setting(0);
    cf_set_t legacy = cs;
    legacy.expression = 0;
    cfs_sanitize(&legacy);
    assert(legacy.expression == CF_EXPR_DEFAULT && legacy.vel == cs.vel);

    /* Simultaneous chord selector and root. Only the root owns the strike. */
    web_expression(root, 34, 67);
    web_expression(major, 123, 127);
    keys((1u << major) | (1u << root), 80);
    unsigned chord_count = expect_strikes(cs.vel);
    assert(chord_count >= 4 && !cf_channel_pressure);
    keys(0, 80);

    setting(CF_EXPR_AT);
    keys(1u << root, 80);
    assert(expect_strikes(cs.vel) == chord_count && cf_channel_pressure == 67);
    keys(0, 80);
    assert(!cf_channel_pressure);

    setting(CF_EXPR_VEL | CF_EXPR_AT);
    cs.fx.on |= 1u << CFX_FILTER;
    cs.fx.v[CFX_FILTER][1] = 40;
    cf_fx_apply(&cs.fx);
    keys(1u << root, 80);
    assert(expect_strikes(34) == chord_count && cf_channel_pressure == 67);
    int32_t low = cfx.fg_t;
    web_expression(root, 34, 127);
    web_step(45);
    assert(cf_channel_pressure == 127 && cfx.fg_t > low);
    assert(cs.fx.v[CFX_FILTER][1] == 40);
    keys(0, 80);
    assert(cfx.fg_t == CFX_G[40] && !cf_channel_pressure);

    /* K4 selects a saved target. Feed a reproducible stereo tone through
     * the two pressure-only effects and check full-pressure DSP and release. */
    for (unsigned target = AT_TREMOLO; target < AT_COUNT; ++target) {
        web_enc(EMU_E_K4, 1);
        web_step(900);
        assert(cf_at_amount[target] == cf_at_defaults[target]);
        unsigned hints = cs.seq_hint;
        cs.seq_hint = 0;
        cf_settings_load();
        assert(cs.seq_hint == hints);
        keys(1u << root, 80);
        assert(cf_channel_pressure == 127 && cfx.fg_t == CFX_G[40]);
        if (target == AT_DRIVE || target == AT_PHASER) {
            unsigned base = cs.fx.v[target == AT_DRIVE ? CFX_DIST : CFX_PHASER][target == AT_DRIVE ? 0 : 1];
            unsigned value = base + (target == AT_DRIVE ? 50 : 60);
            if (value > 100) value = 100;
            if (target == AT_DRIVE) assert(cfx.dg == CFX_DRIVE[value]);
            else assert(cfx.pdep == (int)value * 32 * 256 / 100);
        } else {
            int32_t left[128], right[128];
            for (unsigned i = 0; i < 128; ++i) right[i] = left[i] = osc_sine(i * 97400000u);
            cfx_pressure_mod(left, right, 128, target, 127);
            unsigned changed = 0;
            for (unsigned i = 0; i < 128; ++i) {
                assert(left[i] == right[i] || i < 32);
                changed += left[i] != osc_sine(i * 97400000u);
            }
            assert(changed > 64);
        }
        keys(0, 80);
        assert(!cf_channel_pressure);
        expect_strikes(34);
    }
    web_enc(EMU_E_K4, 1); /* Phaser -> Multi */
    web_step(40);
    assert(cs.at_mode == AT_COUNT);
    web_enc(EMU_E_SELECT, -1); /* AT Mix */
    web_step(40);
    web_enc(EMU_E_K1, -4); /* browse cutoff */
    web_step(40);
    web_enc(EMU_E_K2, -2);
    web_step(900);
    assert(cf_at_amount[AT_CUTOFF] == 90 && cf_at_amount[AT_TREMOLO] == 25);
    web_enc(EMU_E_SELECT, 1);
    web_step(40);
    web_enc(EMU_E_K4, -5); /* solo cutoff */
    web_step(900);
    assert(cf_at_amount[AT_CUTOFF] == 90 && !cf_at_amount[AT_TREMOLO]);
    cs.at_amount[AT_CUTOFF] = cs.at_mode = cs.at_mix_mask = 0;
    cf_settings_load();
    cf_send_config();
    assert(cs.at_amount[AT_CUTOFF] == 90 && cs.at_mix_mask == 3 && cs.at_mode == AT_CUTOFF);
    web_enc(EMU_E_K4, 5);
    web_step(40);
    assert(cf_at_amount[AT_CUTOFF] == 90 && cf_at_amount[AT_TREMOLO] == 25);
    web_enc(EMU_E_K4, -5);
    web_step(40);

    /* An unmapped landing expires and a short tap still sounds and releases. */
    web_expression(root, 0, 0);
    keys(1u << root, 8);
    assert(!expect_strikes(cs.vel));
    web_step(40);
    assert(expect_strikes(cs.vel) == chord_count);
    keys(0, 80);
    keys(1u << root, 3);
    keys(0, 80);
    assert(expect_strikes(cs.vel) == chord_count && !cfc.nroots);

    /* A second physical key takes the maximum pressure; a selector cannot. */
    cs.chord.algo = CF_ALG_TOP;
    cf_send_config();
    web_expression(root, 88, 59);
    web_expression(9, 40, 95);
    web_expression(5, 127, 127);    /* chord selector on an analog row */
    keys((1u << root) | (1u << 9) | (1u << 5), 70);
    assert(cf_channel_pressure == 95);
    keys((1u << root) | (1u << 5), 70);
    assert(cf_channel_pressure == 59);
    setting(CF_EXPR_VEL);
    assert(!cf_channel_pressure);
    keys(0, 80);
    while (web_midi_out_take()) {}

    /* MIDI and the keyboard share ownership of the same mapped key. The
     * physical release clears its pressure while MIDI continues to hold it. */
    setting(CF_EXPR_AT | CF_EXPR_VEL);
    keys(1u << root, 40);
    expect_strikes(88);
    web_midi_in(9u | (0x90u << 8) | (60u << 16) | (20u << 24));
    web_step(20);
    keys(0, 40);
    assert(cfc.nroots && !cf_channel_pressure);
    web_midi_in(8u | (0x80u << 8) | (60u << 16));
    web_step(40);
    assert(!cfc.nroots);
    while (web_midi_out_take()) {}

    /* A latched strum carries its original strike after the fingers lift. */
    cs.arp_keep = 1;
    cs.arp.mode = CF_ARP_STRUM_DOWN;
    cs.arp.strum_ms = 50;
    cs.arp.latch = 1;
    cu.arp_on = 1;
    cf_send_config();
    web_step(50);
    web_expression(root, 112, 0);
    keys((1u << major) | (1u << root), 25);
    assert(expect_strikes(112) >= 1);
    keys(0, 15);
    web_expression(root, 20, 0);
    web_step(200);
    assert(expect_strikes(112) >= 2);
    keys(0, 80);

    printf("Expression: fixed, pressure-only, dynamic chord/bass, cutoff, fallback, tap, overlap and strum passed\n");
    return 0;
}
