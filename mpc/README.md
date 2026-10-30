# 8cee Console 4K on MPC Live 3 (VST2, Linux AArch64)

Channel-4K, Bus-4K and Listen-4K as native insert effects for MPC OS standalone, built on
[sd88me/mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins). Independent JUCE-free ports of the desktop algorithms, with MPC touchscreen pages and Q-Links. Factory preset names and converted settings come from the desktop sources; the oversampling filters and parameter smoothing differ, so sample-identical desktop audio is not claimed.

| Plug-in | Pages | Presets |
|---|---|---|
| Channel-4K | CHANNEL (drive, phase, HPF/LPF, output, EQ/DYN in) · EQ (4 bands, Brown/Black) · DYNAMICS (comp, gate, routing, mix) | 18 |
| Bus-4K | one page: threshold, make-up, mix, console drive, ratio, attack, release, sidechain HPF | 14 |
| Listen-4K | one page: input, release, crunch, mix, output, voicing | 12 |

Presets are in the plug-in's **PRESET** menu on the MPC.

## Builds
`.github/workflows/mpc.yml` builds **AArch64 only** for MPC Live 3. ARMv7 is outside the current scope.
The toolkit and reusable workflow are pinned to `adc48a35e8e190cd4c7a1adb20da5a4cce2fdea4`.
The toolkit's `gen2_only` packaging switch creates only `*-mpc-aarch64.zip` installers.
Regression tests and generated-file checks must pass before packaging.

Download them under **Actions → "MPC plug-ins" → latest run → Artifacts**. Each zip has the plug-in folder, `install.sh`,
`uninstall.sh` and an `INSTALL.md`.

## Installing
MPC OS has no official way to add VST plug-ins, so installing needs a device with root access (see the toolkit's README and
INSTALL.md). The toolkit's desktop installer app can copy the plug-in over and register it, so nothing has to be typed.
Back up `MPC.settings` first (the installer does this). MPC has to restart once to see a new plug-in.

## Notes
- Audio runs at 44.1 kHz in 128-frame blocks, 16-bit inside the plug-in (MPC's own format for these).
- The 2x oversampling adds about 0.35 ms of latency.
- Frequency, Q, ratio and release controls are log-scaled 0..100 positions; the screen shows the real value (Hz, s, :1).
- No gain-reduction meters on the MPC pages yet.
- Device compatibility is not yet verified on physical MPC Live 3 hardware. CI host tests and a successful AArch64 build do not substitute for a device test.

Regenerate `params.json` / `presets.json` after changing presets in the desktop plug-ins: `python3 mpc/tools/gen_ports.py`.

## Development checks

```sh
python3 mpc/tools/gen_ports.py --check
cmake -S tests -B build-tests -DMPC_VST_PATH=/path/to/mpc-vst-plugins -DECEE_SANITIZERS=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

`gen_ports.py` also generates each engine's `param_defs.h`: defaults, limits and choice flags use the same definitions as the MPC UI and presets. Parameter order and plug-in IDs remain unchanged. Malformed/nonfinite values are ignored; finite out-of-range values are clamped, including restored state. Save chunks use nine significant digits for float round trips.

Build with the pinned toolkit on Linux with Docker/QEMU and Pillow:

```sh
export MPC_VST=/path/to/mpc-vst-plugins
"$MPC_VST/tools/build_port.sh" mpc/channel4k/vst.json aarch64
"$MPC_VST/tools/test_port.sh" mpc/channel4k/vst.json
```

Repeat for `bus4k` and `listen4k`, or use the repository workflow to build and package all three.
Before treating a zip as device-verified: install with the toolkit, check every page/Q-Link and preset, process stereo audio, save/reload an MPC project, check CPU usage with the toolkit benchmark, and test uninstall. Record the MPC OS version and exact build commit with those results.
