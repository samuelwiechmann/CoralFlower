# CoralFlower Expressive for RemoteFM1

Import `coralflower_expressive-1.1.0.rfm1` using **Add firmware…** in RemoteFM1. Under **GLO → Expression**, K1 enables velocity, K2 sets hardware strike sensitivity, K3 enables aftertouch and K4 (SOLO) assigns one effect at 100% and clears the other amounts. The host supplies processed velocity and pressure, so the hardware calibration curve applies at the sensor. Pressure follows the strongest eligible held key and processes the stereo mix.

| K4 target | Playing response |
| --- | --- |
| Cutoff | Enable Filter in FX, choose low-pass and try cutoff 40. Full pressure adds 40 steps. |
| Tremolo | Pressure increases the depth of a 5 Hz pulse. |
| Freq shift | Pressure blends in an upward frequency shift reaching about 39 Hz. |
| Drive | Enable Distortion in FX. Pressure adds up to 50 drive steps. |
| Phaser | Enable Phaser in FX and raise its mix. Pressure adds up to 60 sweep-depth steps. |

Under **GLO → AT Mix**, K1 (FX) browses effects and K2 (AMT) sets each depth from 0% to 100% in 5% steps. All nonzero routes respond together. Cutoff starts at 100%. A version 1.0 assignment imports at 100% on its previous target. Back up before flashing, especially if returning to older firmware with a shorter settings record. Target selection is saved alongside the existing expression settings. On a controller, the host must supply key velocity and lean through the RemoteFM1 panel interface.

## Rebuild

This local project recovers Expressive revision `485d60a74fd223b55b3c21f3338cdcf1dbba2f5b` and adds selectable pressure routing. The RemoteFM1 adapter uses [FM1VST](https://github.com/Quixotic7/FM1VST/tree/c6f8de7b233ef4ef0343e4421083a8286ce57768) at that exact revision. The manifest identifies local source. Keep this project with the instrument.

From the project root, with Python, Pillow, fontTools, Bash and Emscripten 6.0.13 available:

```sh
python3 -c 'import sys; sys.path.insert(0, "tools"); import build; build.generate()'
export FM1VST=/absolute/path/to/FM1VST
bash remote/build.sh
node remote/play-test.mjs build/remote/coralflower_expressive-1.1.0.rfm1
```

The play test boots independent instances through the public RFM1 interface. It turns the panel encoders, verifies a strike velocity of 85 via outgoing MIDI, and compares audio at 0% and 100% lean for each target and checks a layered cutoff/tremolo patch. The source-level integration test in `tests/cf_expression_test.c` exercises saved settings and release. Device and iOS playback require a separate listening test.

