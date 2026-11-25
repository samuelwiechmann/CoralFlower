# CoralFlower Expressive for the M-VAVE FM-1

This experimental fork adds local key velocity and channel pressure. Both switches start off, including when loading settings from stock CoralFlower. The package uses CoralFlower's existing FM-1_930 identity, loader and flash layout. Its splash version reads **CoralFlower Expressive**. Installation and sustained playback on physical units are pending.

## Install and restore

1. Connect one FM-1 directly to a computer with a USB data cable. In desktop Chrome or Edge, open the CoralFlower installer. If the device already runs CoralFlower, use **Back up and restore** and keep a copy of the backup and the version you are replacing. The installer also supports backups from ChoralRoot and Felucca. Vendor V15 sounds cannot be backed up by this tool. [CoralFlower installer](https://ficusficusov.github.io/CoralFlower/)
2. Use **Choose a .fwsc file** and select the supplied `coralflower-expressive.fwsc`. Verify the filename and compare its SHA-256 with the bundle's `SHA256SUMS`. Press **Install on your FM-1**. Keep the device powered and connected through the restart.
3. To return to CoralFlower, select the prior release in the same installer, install it and restore your pre-test backup if needed. To return to the manufacturer's firmware, obtain the official FM-1 V15 file and use **Return to the official firmware** → **Choose the official FM-1.fwsc** → **Back up and return to official V15**. Keep your CoralFlower backup for a later reinstall. [M-VAVE downloads](https://www.m-vave.com/download)
4. If writing was interrupted, follow the installer's interrupted-update instructions with the same package. For manual CoralFlower update mode, switch off, hold **OCT−**, and switch on. Three flashes followed by a black screen indicate that the loader is waiting. The installer has further recovery instructions for a single long flash.

## Play and simulate

- Open **GLO → Expression**. K1 enables velocity. K2 adjusts strike sensitivity from −3 to +3, with 0 as the reference response and positive values making lighter strikes louder. K3 enables aftertouch independently. The existing **GLO → Play → Velocity** value still controls fixed strikes and acts as fallback when sensing misses a landing.
- To hear pressure clearly, enable the FX **Filter**, select low-pass, and set its cutoff around 40. Strike a lower-row root, hold for about a quarter second, then lean further into it. The strongest held musical key opens the filter by up to 40 cutoff steps, capped at 99. Release returns it to the patch setting. This filter processes the mix, so other parts playing through it follow the same movement. The meter on the Expression page shows channel pressure.
- All 27 keys can supply strike velocity when assigned a musical role. The 20 keys F3–C5 sense strike and pressure through the analog rows. C♯5–G5 use the PA10 probe with a fixed calibration curve and supply strike only. Chord selectors and editor gestures retain their control roles. A missing landing falls back after 20 ms from first closure at the next input pump, or sooner if the key is released.
- Physical chords and bass share the root's strike. Arp and strum retain their originating velocities for scheduled notes. Existing glide and retrigger rules still decide when a voice receives a fresh attack. Physical velocities appear in the existing outgoing MIDI note-ons when enabled. Channel pressure controls the local filter. The upstream sequencer format keeps pitch and timing and plays back at its configured velocity. External MIDI input follows the existing Keys, Roots and Direct modes.
- In the bundle, serve the extracted `emulator` directory with `python3 -m http.server 8000 --directory emulator` and open **http://localhost:8000/play/**. Press **Start**, enable the instrument settings above, and open **Simulate key expression**. Adjust **Next strike** before playing and **Held pressure** while holding a panel or computer key. The sliders supply processed values directly. Raw sensor response and sensitivity curves are exercised by the host sensor test.

## Implementation and builds

CoralFlower starts at `46790b943bdb5e4143212d83dc495c832eefdbb8`. Velo established the driven carbon-contact measurement. CTL-1 uses fm1-base's touch layer and the strongest held key for channel pressure. This fork reuses ChoralRoot's GPL-3.0-only integer port at `47a2d8048fb43b35a667135836bce46da7310ad9`, which fits CoralFlower's timer and unity build. The fm1-base touch library assumes its own platform, math and event lifecycle. The selected port keeps CoralFlower's digital debounce and tap handling. [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1) · [Velo](https://github.com/charlesvestal/fm1-velocity) · [fm1-base](https://github.com/charlesvestal/fm1-base) · [CTL-1](https://github.com/charlesvestal/fm1-ctl)

The HAL multiplexes the ADC between the two rows, top-row probe, MASTER and battery. It restores driven pins before the next digital read. The sensor in `cr_sense.c` produces per-key landings and pressure. `cf_expression.c` handles musical eligibility, fallback and channel aggregation. The audio queue carries strike velocity alongside the original key age. The player keeps strike information independent of global settings. Both controls off return ADC ownership to the original pot and battery path. The expression settings occupy a tagged byte of existing padding in the 188-byte settings record.

Build on x86-64 Linux with Python, Pillow and fontTools. This prototype used JieLi's 20260730.1 toolchain, the vendored SDK files, Pillow 13.0.0, fontTools 4.61.1 and Emscripten 3.1.74. From the checkout, fetch the compiler with `sh tools/get_toolchain.sh`, then run `sh build.sh` for `build/coralflower.fwsc`. Activate the Emscripten environment and run `sh tools/emu/web/build_web.sh`, followed by `sh tools/make_site.sh build/site`. The package is built from the same tracked sound assets as the baseline. Copyright notices and redistribution terms are in LICENSING.md and LICENSES.

## Measured results

| Linked region | Upstream | Expressive | Change |
|---|---:|---:|---:|
| Application in flash | 366,116 B | 371,652 B | +5,536 B |
| Static RAM, data + BSS | 92,284 B | 93,308 B | +1,024 B |
| Audio and asset pool | 322,828 B | 322,828 B | 0 B |

The build leaves 4,996 B of static RAM and 21,236 B of pool capacity. These are link-time figures. Stack high-water and interrupt timing need measurements on the FM-1.

- **Sensing and routing.** The 133-sample sensor suite covers soft and firm landings, grazes, bounce, independent keys, curve endpoints, quick top-row releases and pressure settling. The emulator integration test covers the three settings via panel controls, persistence, old settings, fixed and dynamic chord/bass output, cutoff modulation, release, short taps, missing landings, overlapping MIDI ownership and latched strums.
- **Compatibility.** Upstream chord tests (94 checks), backups (132), sequencer gestures and storage tests passed. Across 18 seeded stuck-note runs, all 2,178 inspected states had zero stray held notes and each run ended silent. With expression disabled the upstream and fork WAVs matched byte-for-byte over 243,432 stereo frames. The fork's WASM and native emulator also produced identical audio and state dumps.
- **Host CPU.** A native emulator benchmark rendered 1,048,576 stereo frames with changing roots, arpeggiator, bass, sequenced drums, filter, chorus and reverb. Median process CPU over five runs was 0.231 s upstream, 0.244 s with expression off and 0.234 s with simulated expression on. Disabled PCM and MIDI hashes matched upstream and all three configurations produced 179 note-ons. These measurements include the emulator and hashing. They omit physical ADC interrupts.
- **Browser audio.** The standalone WASM render test measured a 0.076 ms mean and 0.255 ms 99th percentile per 128-frame quantum against a 2.90 ms audio budget. A concurrent build briefly drove a host timing sample to 32.9 ms. The isolated rerun peaked at 0.797 ms. Headless Chromium booted the page and started its AudioWorklet. The installer accepted the experimental package in its local file picker.

The gesture and expression compiler commands are in `.github/workflows/check.yml`. To reproduce the host benchmark from the repo root after building generated headers:

```sh
cc -O2 -w -std=gnu11 -Itools/emu/web -Itools/emu/web/compat -Ibuild/gen \
  -Ifirmware/src -DEMU_WEB tests/cf_expression_bench.c -o build/host/expression_bench
build/host/expression_bench
build/host/expression_bench on
```

## First hardware session

1. Verify backup and restore on one unit. Record device revision, original firmware, build checksum and settings.
2. Monitor outgoing USB MIDI. Compare soft, normal and firm strikes per key, including off-centre strikes and fast repetitions. Use a layout where the key under test is a root. Check the C5/C♯5 transition, simultaneous selectors and roots, short taps, held arp/strum and octave changes.
3. Test pressure with velocity both off and on. Record and listen for settling drift, audible cutoff stepping, release to baseline and sustained two-key handoffs. Compare ranges on a second unit before changing calibration.
4. Under dense chords, arp, sequencer, USB MIDI and FX, compare feature-off and feature-on scan cost, latency and audio underruns. Check MASTER response, battery readings, key LEDs and panel controls during upper-row sampling. Reboot to verify settings and sound/pattern retention, then exercise rollback.
