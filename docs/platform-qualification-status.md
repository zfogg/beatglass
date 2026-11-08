# Platform qualification status

This matrix records development evidence and the remaining gates. A build or headless test does not establish native desktop, audio, or overlay support.

| Platform | Executable preparation checks | Native gates still required |
| --- | --- | --- |
| Windows | Configure with the pinned vcpkg preset; build the selected configuration; run `ctest --preset vcpkg-core` or the matching Visual Studio configuration; run the strict documentation checks. | Real WASAPI loopback permission and routing, device removal/recovery, fullscreen and multi-monitor overlay behavior, tray recovery, DPI changes, and packaged runtime dependencies on supported Windows versions. |
| macOS | Configure with the platform toolchain and Homebrew/isolated dependencies; build and run CTest; run strict documentation checks. | Real Core Audio capture permission prompts, output-device switching and recovery, Spaces/fullscreen behavior, menu-bar recovery, Retina/DPI changes, Intel and Apple Silicon runtime behavior, and packaged signing/runtime dependencies. |
| Linux | Configure with `-DBEATGLASS_BUILD_PIPEWIRE=ON`; build serially when memory is constrained; run CTest, including optional virtual-graph tests; run strict documentation checks. | Real PipeWire device routing and permissions, device loss/replacement, X11 window-manager behavior, GNOME and KDE Wayland probing, layer-shell refusal/fallback, monitor/DPI changes, and recovery from an unavailable overlay capability. |

For every native run, record the OS version, kernel or build number, compiler, Qt version, audio backend/session version, display server or compositor, output device, build identity, exact commands, and observed limitations. Until those records exist for the relevant platform scenarios, support language must remain experimental.

## Issue #76 UI gate

The DJ workflow must remain usable when overlay capability is unavailable or denied. Native checks must verify that the main controls stay recoverable, the selected output device and capture state remain visible, stale analysis is cleared after loss, and an unavailable overlay never causes an invisible or stranded window. Click-through, tray/menu-bar recovery, fullscreen behavior, monitor changes, and reduced-motion behavior are separate platform observations; a mockup or window flag alone does not satisfy them.
