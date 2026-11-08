# Platform qualification status

This matrix records development evidence and the remaining gates. A build or headless test does not establish native desktop, audio, or overlay support.

| Platform | Current integrated/preparation evidence | Native gates still required |
| --- | --- | --- |
| Windows | `tests/windows/run_qualification.ps1` records build identity, configures/builds the x64 desktop targets, discovers tests, verifies artifacts, and can run CTest with `-RunTests`; endpoint-loss state and display selection/recovery have unit/QML coverage. | Real WASAPI loopback permission and routing, device removal/recovery on hardware, fullscreen and multi-monitor overlay behavior, tray recovery, DPI changes, and packaged runtime dependencies on supported Windows versions. |
| macOS | `tests/macos/run_qualification.sh` now enables the production desktop-analysis bridge and records build/evidence metadata; shell syntax and local diff checks pass. | Re-run the production qualification on `zachbook-pro`; verify real Core Audio capture permission prompts, output-device switching/recovery, Spaces/fullscreen behavior, menu-bar recovery, Retina/DPI changes, Intel and Apple Silicon runtime behavior, and packaged signing/runtime dependencies. |
| Linux | Configure with `-DBEATGLASS_BUILD_PIPEWIRE=ON`; build serially when memory is constrained; run CTest, including optional virtual-graph tests; run strict documentation checks. | Real PipeWire device routing and permissions, device loss/replacement, X11 window-manager behavior, GNOME and KDE Wayland probing, layer-shell refusal/fallback, monitor/DPI changes, and recovery from an unavailable overlay capability. |

For every native run, record the OS version, kernel or build number, compiler, Qt version, audio backend/session version, display server or compositor, output device, build identity, exact commands, and observed limitations. Until those records exist for the relevant platform scenarios, support language must remain experimental.

## Issue #76 UI gate

The DJ workflow must remain usable when overlay capability is unavailable or denied. Native checks must verify that the main controls stay recoverable, the selected output device and capture state remain visible, stale analysis is cleared after loss, and an unavailable overlay never causes an invisible or stranded window. Click-through, tray/menu-bar recovery, fullscreen behavior, monitor changes, and reduced-motion behavior are separate platform observations; a mockup or window flag alone does not satisfy them.

The Linux capture gate also requires a documented sample-position clock mapping. A PipeWire graph-clock snapshot or callback arrival time is diagnostic evidence only; it must not be exposed as a frame-accurate anchor until buffer timing, converter/queue delay, clock domain, drift, and uncertainty have been measured on the target backend and device.
