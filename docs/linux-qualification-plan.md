# Linux qualification plan

This plan records the evidence still required before Linux support can be claimed. WSL, CI, headless sessions, and virtual PipeWire graphs are useful development checks but do not satisfy the native desktop gates.

## Runnable preparation checks

- Run `python tests/qualification/linux_environment_report.py` and retain its JSON output. On non-Linux hosts it exits with status 2 and reports `native_linux_claim: false`; that is an environment blocker, not a failed product test.
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

Use the [native Linux evidence template](linux-native-evidence-template.md) for each session so environment identity, commands, observations, and limitations remain reviewable.
