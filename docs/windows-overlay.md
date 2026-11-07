# Windows overlay controller

The provisional desktop-private controller sets frameless, transparent,
always-on-top Qt window properties and can enable Windows click-through. It
requires the caller to supply a separate control window. It refuses click-through
when that window is hidden or minimized, and unlocks on hide, minimize, close,
control destruction or controller destruction. It clamps an off-screen window
to an available screen in Qt logical coordinates, including after screen removal.

The controller is not connected to the product UI. The shared UI must provide
discoverable unlock and hide controls, a supported shortcut, opacity and monitor
controls, and persistence before this becomes a usable overlay. There is no
claim that topmost windows appear above every fullscreen application. Opacity,
DPI changes, monitor unplug and actual pointer pass-through need interactive
checks, including mixed-DPI displays; the automated checks do not replace them.

Windows 11 Home 10.0.26300 x64, MSVC 19.44, Qt 6.8.3 Debug: configured with
the Qt SDK and pinned codec dependency tree described in `docs/progress.md`;
`cmake --build "$env:TEMP/beatglass-overlay-build" --config Debug --target windows_overlay_tests -j 4`
and `ctest --test-dir "$env:TEMP/beatglass-overlay-build" -C Debug -R '^windows_overlay$' --output-on-failure`
passed. The test created native windows, checked the Windows extended topmost and
transparent-for-input styles, and exercised hide/minimize/destruction unlock and
off-screen geometry recovery. The initial CTest run timed out because the Windows
Qt platform plugin had not been deployed; the target now copies that plugin and
the rerun passed without `QT_PLUGIN_PATH`. MSBuild warned that using a temporary
directory for the build can affect incremental builds. A combined metadata and
overlay test initially failed to launch metadata with `0xc0000135`; after adding
the existing codec tree's `x64-windows/debug/bin` and `bin` to `PATH`,
`ctest --test-dir "$env:TEMP/beatglass-overlay-build" -C Debug -R '^(metadata_worker|windows_overlay)$' --output-on-failure`
passed 2/2. The Docker Sphinx build passed with warnings treated as errors.
