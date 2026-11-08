# Platform qualification status

This matrix records development evidence and the remaining gates. A build or headless test does not establish native desktop, audio, or overlay support.

| Platform | Current integrated/preparation evidence | Native gates still required |
| --- | --- | --- |
| Windows | `tests/windows/run_qualification.ps1` records build identity, configures/builds the x64 desktop targets, discovers tests, verifies artifacts, and can run CTest with `-RunTests`; endpoint-loss state and display selection/recovery have unit/QML coverage. | Real WASAPI loopback permission and routing, device removal/recovery on hardware, fullscreen and multi-monitor overlay behavior, tray recovery, DPI changes, and packaged runtime dependencies on supported Windows versions. |
| macOS | `tests/macos/run_qualification.sh` enables the production desktop-analysis bridge and records build/evidence metadata; `tests/macos/verify_qualification_artifacts.sh` verifies the retained bundle; shell syntax and local diff checks pass. | Re-run the production qualification on `zachbook-pro`; verify real Core Audio capture permission prompts, output-device switching/recovery, Spaces/fullscreen behavior, menu-bar recovery, Retina/DPI changes, Intel and Apple Silicon runtime behavior, and packaged signing/runtime dependencies. |
| Linux | Configure with `-DBEATGLASS_BUILD_PIPEWIRE=ON`; build serially when memory is constrained; run CTest, including optional virtual-graph tests; run strict documentation checks. | Real PipeWire device routing and permissions, device loss/replacement, X11 window-manager behavior, GNOME and KDE Wayland probing, layer-shell refusal/fallback, monitor/DPI changes, and recovery from an unavailable overlay capability. |

For every native run, record the OS version, kernel or build number, compiler, Qt version, audio backend/session version, display server or compositor, output device, build identity, exact commands, and observed limitations. Until those records exist for the relevant platform scenarios, support language must remain experimental.

Accessibility readiness must separately record reduced-motion behavior, click-through recovery, and keyboard-accessible recovery controls.

Runtime evidence must explicitly retain the audio backend/session version, display server or compositor, exact commands, and observed limitations.

The scenario matrix must cover permission handling, output-device selection/loss, and recovery or reacquisition behavior on each applicable platform.

Detailed handoff checklists: [Windows](windows-qualification.md), [macOS](macos-qualification.md), and [Linux](linux-qualification-plan.md).

Each native scenario record must retain its capture timestamp, supporting logs, and a screenshot or recording path; a status without that evidence remains unqualified.

## Issue #76 UI gate

The DJ workflow must remain usable when overlay capability is unavailable or denied. Native checks must verify that the main controls stay recoverable, the selected output device and capture state remain visible, stale analysis is cleared after loss, and an unavailable overlay never causes an invisible or stranded window. Click-through, tray/menu-bar recovery, fullscreen behavior, monitor changes, and reduced-motion behavior are separate platform observations; a mockup or window flag alone does not satisfy them.

The Linux capture gate also requires a documented sample-position clock mapping. A PipeWire graph-clock snapshot or callback arrival time is diagnostic evidence only; it must not be exposed as a frame-accurate anchor until buffer timing, converter/queue delay, clock domain, drift, and uncertainty have been measured on the target backend and device.

## Local evidence audit (2026-11-08)

The preparation scripts are executable on their respective hosts, but this Windows session is not a substitute for native qualification: it cannot produce WASAPI, Core Audio, or PipeWire device evidence. The current actionable handoff is therefore:

| Evidence class | Windows session result | Required native owner/action |
| --- | --- | --- |
| Build identity and artifact inventory | Prepared by `tests/windows/run_qualification.ps1`; no native-device run claimed here | Run on supported Windows hardware with a logged-in desktop session and retain the generated evidence directory. |
| macOS production bridge and overlay | Not runnable on Windows; script/checklist remains the source of truth | Run `tests/macos/run_qualification.sh` on `zachbook-pro`, then complete the permission, device-loss, overlay, and packaged-runtime observations above. |
| Linux PipeWire timing and routing | Not runnable on Windows | Run the Linux qualification plan with a real graph and record clock-domain, queue-delay, drift, and uncertainty measurements. |

This audit intentionally records unavailable evidence as pending rather than treating portable builds, fake backends, or headless tests as native support.
