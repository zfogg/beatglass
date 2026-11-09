# Linux qualification plan

This plan records the evidence still required before Linux support can be claimed. WSL, CI, headless sessions, and virtual PipeWire graphs are useful development checks but do not satisfy the native desktop gates.

Retain `qualification-manifest.txt`, `qualification-artifacts.sha256`, `native-capture-evidence.json`, `native-recovery-evidence.json`, `native-output-evidence.json`, and `native-display-evidence.json` with each native run.

## Runnable preparation checks

- Run `python tests/pipewire/check_linux_native_prerequisites.py --report evidence/linux-pipewire-preflight.json` to record a machine-readable prerequisite result. On non-Linux hosts this exits successfully with `status: pending`; it never claims native capture evidence.
- The probe checks daemon reachability and the independent `pkg-config` headers/linker capability, so a host with a running daemon but no development package cannot be mistaken for a build-ready qualification host.
- The generated report includes the exact next probe, CMake configuration, and native-runner commands, making a pending Windows/macOS handoff actionable without implying that those commands ran.
- Build with `-DBEATGLASS_BUILD_PIPEWIRE=ON` and run the complete CTest suite.
- Run the isolated virtual-sink graph tests for capture, source replacement, wrong-link injection, interruption, and bounded queue overflow.
- Run the strict API documentation checks and record the indexed header count.
- Record PipeWire, WirePlumber, compiler, kernel, Qt, and session versions with each result.

## Native Linux gates

For each session, record the exact distribution, kernel, compositor/session version, display server, PipeWire and WirePlumber versions, output device, and build identity.

- Verify output-device discovery, permission behavior, selected-sink routing, device removal, replacement, and recovery on real hardware.
- Verify that graph loss, format change, queue overflow, and discontinuity clear published analysis and require a new acquisition epoch.
- Exercise an X11 session with a real window manager, including stacking, input-shape behavior, monitor/DPI changes, fullscreen applications, and recovery from an off-screen overlay.
- Exercise GNOME and KDE Wayland sessions separately. Probe layer-shell availability and compositor refusal; fall back to an ordinary compact window when the required capability is absent.
- Verify that no fallback silently captures a microphone or a different output sink, and that the user can always recover the main controls.

Until these observations are recorded on actual Linux desktops, the project should describe Linux capture and overlay as experimental and unsupported.

Review acceptance also verifies that every `PASS` scenario points to a retained,
non-empty evidence file beneath the submitted report directory; a status string
alone cannot promote native evidence.

The reviewer additionally requires matching platform/manifest identity, a
40-character build commit, complete provenance, exact artifact counts, and the
full six-scenario inventory before a platform can be accepted.

Use the [native Linux evidence template](linux-native-evidence-template.md) for each session so environment identity, commands, observations, and limitations remain reviewable.
