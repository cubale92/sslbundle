# Source and dependency licensing

8cee Console 4K is an independent console-inspired implementation. No claim is made that Solid State Logic proprietary plugin source is open source, included, or licensed to this project. The product is not endorsed by Solid State Logic.

The inspected repository has no top-level LICENSE. This change does not assign a licence to the owner's existing work. Public availability alone is not a complete redistribution licence. A project licence remains an owner decision before public distribution.

## Pinned dependencies

- JUCE 8.0.4 (`51d11a2be6d5c97ccf12b4e5e827006e19f0555a`) is dual-licensed under AGPLv3 or the commercial JUCE licence. Its bundled components have additional notices, including the VST3 SDK shipped at that revision. See https://github.com/juce-framework/JUCE/blob/8.0.4/LICENSE.md . Desktop source is preserved; desktop builds are currently manual-only.
- MPC uses `sd88me/mpc-vst-plugins` at `adc48a35e8e190cd4c7a1adb20da5a4cce2fdea4`. Its root LICENSE and the vendored force-shadow LICENSE are MIT, copyright 2026 sd88me. The original notice is retained in `mpc/THIRD_PARTY_NOTICES.txt` and included in installer packages. Other toolkit components retain their own terms; this is not a blanket relicensing of the toolkit or third-party plugins.
- The MPC engine does not link JUCE or the Steinberg VST2 SDK; it uses the pinned toolkit's wrapper. This dependency inventory is not a claim of official Akai support or an assessment of every trademark/device agreement.

Do not label builds as SSL code, officially supported MPC plugins, or verified on hardware until the corresponding evidence exists.
