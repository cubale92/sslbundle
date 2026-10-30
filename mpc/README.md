# 8cee Console 4K on MPC (VST2, Linux ARM)

Channel-4K, Bus-4K and Listen-4K as native insert effects for MPC OS standalone, built on
[sd88me/mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins). Same DSP and the same factory presets as the desktop
plug-ins (rewritten without JUCE, which MPC OS can't load), with MPC touchscreen pages and Q-Links.

| Plug-in | Pages | Presets |
|---|---|---|
| Channel-4K | CHANNEL (drive, phase, HPF/LPF, output, EQ/DYN in) · EQ (4 bands, Brown/Black) · DYNAMICS (comp, gate, routing, mix) | 18 |
| Bus-4K | one page: threshold, make-up, mix, console drive, ratio, attack, release, sidechain HPF | 14 |
| Listen-4K | one page: input, release, crunch, mix, output, voicing | 12 |

Presets are in the plug-in's **PRESET** menu on the MPC.

## Builds
`.github/workflows/mpc.yml` builds both CPUs on every push to `mpc/`:
- `*-mpc-armv7.zip`: Gen1, i.e. MPC Live, Live II, One, X, Key 61 and Force (32-bit ARM, glibc 2.31).
- `*-mpc-aarch64.zip`: Gen2, e.g. MPC Live III (64-bit ARM).

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
- Not run on a device yet: offline host tests (ASan/UBSan) pass for all three, levels match the desktop plug-ins.

Regenerate `params.json` / `presets.json` after changing presets in the desktop plug-ins: `python3 mpc/tools/gen_ports.py`.
