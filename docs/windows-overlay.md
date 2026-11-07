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
DPI changes and monitor unplug still need interactive checks; the automated
checks do not replace full product qualification.

The shared desktop owns both windows and must construct the controller and call
its methods on the Qt GUI thread. It must keep the control window visible and
not minimized while click-through is active, expose unlock/hide there, and
provide a shortcut independently of overlay input. `setClickThrough` reports
whether it accepted the requested state; the UI must reflect the returned state
and subscribe to `clickThroughChanged` so automatic unlock when the control
window becomes unavailable is reflected in the UI.
The control and overlay must be distinct windows; click-through is rejected if
they are the same object, because that would disable the escape control itself.

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

An opt-in native input test was run with `BEATGLASS_NATIVE_OVERLAY_TEST=1` and
the same `windows_overlay` CTest command. It used `SendInput` on controlled Qt
windows, verified that clicks reached the overlay while unlocked, the window
beneath while click-through was enabled, and the overlay again after the control
window was hidden. It repeated the unlocked/locked checks over a standard Qt
fullscreen window. A second test delivered clicks through the overlay on both
active displays: LG ULTRAGEAR at Qt scale 1 and U2790B at scale 1.5. Qt warned
that a test background window's requested geometry on U2790B was adjusted;
pointer delivery still passed. The test restores the mouse cursor and is skipped
unless explicitly enabled because it moves the cursor and sends clicks. It does
not qualify exclusive fullscreen, physical unplug, DPI changes during a session,
or the as-yet unconnected product UI.

For a Qt-only gate without the decoder stack, the Windows hosted CI job installs
Qt 6.8.3 with `aqtinstall==3.3.0`, configures
`-DBEATGLASS_BUILD_WINDOWS_OVERLAY_TESTS=ON -DBUILD_TESTING=ON`, builds
`windows_overlay_tests` and runs `ctest -R '^windows_overlay$'`. This path was
also configured, built and passed locally on Windows with MSVC 19.44 and the
same Qt SDK. The hosted `windows-overlay` job passed in GitHub Actions run
`42372333794` for commit `89010db`; checks for subsequent merge commits must run
again. The gate does not enable the opt-in
pointer-injection scenarios and is not native product qualification.
