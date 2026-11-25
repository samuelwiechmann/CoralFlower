/* SPDX-License-Identifier: GPL-3.0-only */
/* Build against either checkout using -Itools/emu/web. Time the whole emulator
 * with a live arp, sequenced drums and FX. Hash its PCM and MIDI for comparison. */
#include "emu_web.c"
#include <time.h>
#include <assert.h>

static uint32_t hash_word(uint32_t hash, uint32_t value)
{
    return (hash ^ value) * 16777619u;
}

int main(int argc, char **argv)
{
    uint32_t pcm = 2166136261u, midi = pcm, notes = 0;
    int expression = argc > 1 && !strcmp(argv[1], "on");
    web_boot();
    web_step(1900);
    cu.arp_on = cs.arp_keep = 1;
    cs.fx.on |= (1u << CFX_FILTER) | (1u << CFX_CHORUS) | (1u << CFX_REVERB);
    cs.fx.v[CFX_FILTER][1] = 48;
    sq_st.pat[0].hit[0][0] = 0x1111;
    sq_st.pat[0].hit[1][0] = 0x1010;
    sq_st.pat[0].hit[3][0] = 0x5555;
#ifdef CF_EXPR_DEFAULT
    if (expression) cs.expression |= CF_EXPR_VEL | CF_EXPR_AT;
#else
    assert(!expression);
#endif
    cf_send_config();
    sqp_start(0, 0);
    clock_t start = clock();
    for (unsigned q = 0; q < 8192; q++) {
        unsigned root = (q / 128) % 2 ? 7 : 9;
#ifdef CF_EXPR_DEFAULT
        if (expression)
            web_expression(root, root == 7 ? 40 : 110, (q / 8) % 128);
#endif
        web_keys((1u << 20) | (1u << root));
        web_render(128);
        for (unsigned i = 0; i < 128; i++) {
            pcm = hash_word(pcm, (uint16_t)(int16_t)(web_out_l()[i] * 32768));
            pcm = hash_word(pcm, (uint16_t)(int16_t)(web_out_r()[i] * 32768));
        }
        uint32_t packet;
        while ((packet = web_midi_out_take())) {
            midi = hash_word(midi, packet);
            notes += ((packet >> 12) & 15u) == 9;
        }
    }
    printf("render s %.6f, PCM %08X, MIDI %08X, %u note-ons\n",
           (double)(clock() - start) / CLOCKS_PER_SEC, pcm, midi, notes);
    assert(sqp.on && cfa_k.on && notes > 100);
    return 0;
}
