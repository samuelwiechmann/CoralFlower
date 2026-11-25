# CoralFlower Expressive 1.2

Download the FM-1 hardware package (.fwsc) or the RemoteFM1 instrument (.rfm1) from the fork's Releases page. Back up the device before flashing. In RemoteFM1, use Add firmware.

**GLO → Expression** controls VEL, strike sensitivity, AT and MODE. MODE always offers Cutoff, Trem, Shift, Drive, Phaser and Multi. Turning AT off pauses pressure modulation and keeps your settings.

**GLO → AT Mix** has four knobs. FX selects the effect to edit. AMT sets its depth from 0–100% in 5% steps. IN MIX includes or mutes that effect in Multi. MODE lets you audition one effect or the whole mix while editing. Each effect remembers one amount, shared between solo and Multi. Browsing FX leaves the playing mode alone. Turning MODE on the Expression page to a solo effect also selects that effect for your next edit.

For a first mix, enable the low-pass Filter in FX and set its base cutoff near 40. Choose Multi, include Cutoff at 50% and Trem at 25%, and hold a chord while increasing pressure. Drive and Phaser also require their FX blocks enabled. Shift at 10–20% gives a gentler frequency-shift effect. Settings save automatically. Older single-effect and layered settings migrate on load.

## Rebuild

This local project recovers Expressive revision `485d60a74fd223b55b3c21f3338cdcf1dbba2f5b` and adds selectable pressure routing. The RemoteFM1 adapter uses [FM1VST](https://github.com/Quixotic7/FM1VST/tree/c6f8de7b233ef4ef0343e4421083a8286ce57768) at that exact revision. The manifest identifies the fork revision used for the build.

From the project root, with Python, Pillow, fontTools, Bash and Emscripten 6.0.13 available:

```sh
python3 -c 'import sys; sys.path.insert(0, "tools"); import build; build.generate()'
export FM1VST=/absolute/path/to/FM1VST
bash remote/build.sh
node remote/play-test.mjs build/remote/coralflower_expressive-1.2.0.rfm1
```

The play test boots independent instances through the public RFM1 interface. It turns the panel encoders, verifies a strike velocity of 85 via outgoing MIDI, and compares audio at 0% and 100% lean for each target and checks a layered cutoff/tremolo patch. The source-level integration test in `tests/cf_expression_test.c` exercises saved settings and release. Device and iOS playback require a separate listening test.


