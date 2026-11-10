# macOS overlay development increment

The DJ shell can attach a separate overlay window on macOS. It displays the existing revision-gated live presentation fields; capture and live analysis are not yet connected, so the shell continues to report that monitoring is unavailable. The normal control window offers show, hide, opacity, click-through, unlock, display selection, and three size presets. The overlay can be moved while unlocked and has its own hide button. Closing, hiding, minimizing or destroying the control window unlocks or hides the overlay. An unsuccessful native mode change hides the overlay rather than claiming click-through is off.

The AppKit adapter requests a floating nonactivating window, `ignoresMouseEvents` for click-through, and all-Spaces/fullscreen-auxiliary hints. These are requests, not a guarantee above other applications, fullscreen apps, Stage Manager or all displays. Qt provides opacity and geometry; the controller clamps an off-screen window to an available display when a screen is removed. Geometry and opacity are saved in the application user settings and clamped to an available display on restore; click-through is never restored automatically. Display indices and names are session-local and refresh on screen changes. The overlay shows a discoverable Escape recovery hint while click-through is enabled, and the application shortcut unlocks pointer/keyboard access without requiring a tray or menu-bar item. The shared shell now exposes three overlay size presets, beat-pulse and bounded history presentation surfaces, while the control-window fallback remains the recovery path when a tray or menu-bar item is unavailable. No native Mac UI, click-through, focus, fullscreen, Spaces, multi-monitor, or permission test has been run for this increment; none of those behaviors is qualified yet. The controller and its interface are provisional application-private APIs.

## Portable development checks

The controller exposes `screenIndex` for the shared display selector. Without a saved geometry it places the overlay on the control window's display; valid saved geometry takes precedence. The index follows the overlay's center and is -1 when no display contains it. Display addition, removal, geometry and available-area changes trigger a coalesced clamp and selector refresh. The shared UI must bind its selection to this property and refresh it after display changes. Physical multi-display behavior remains unqualified.

On Windows 11 with MSVC 2022 and a Qt 6.8.3 SDK, with the pinned Catch2 source already downloaded:

```powershell
cmake -S . -B C:/Users/zachf/.codex/builds/beatglass-macos-overlay -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON -DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON -DCMAKE_PREFIX_PATH=C:/Users/zachf/AppData/Local/Temp/beatglass-qt/6.8.3/msvc2022_64 -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/builds/beatglass-macos-fifo/_deps/catch2-src
cmake --build C:/Users/zachf/.codex/builds/beatglass-macos-overlay --config Debug --target macos_overlay_tests --parallel 1
ctest --test-dir C:/Users/zachf/.codex/builds/beatglass-macos-overlay -C Debug -R '^macos_overlay_' --output-on-failure
```

The controller test uses fake native operations and Qt's offscreen platform, and also loads the QML shell with the real controller to verify window attachment and control-to-controller calls. The separate QML test exercises the controls through a fake bridge. These checks cannot establish AppKit behavior. A first QML attempt using synthetic mouse clicks failed under the offscreen Basic style; the test now exercises the controls' clicked signals and the failed click-through path. The final two CTest cases passed. The GitHub macOS overlay job compiles the AppKit source but likewise uses fake-native/offscreen tests; native desktop qualification remains separate.

When the Mac is available, configure with `-DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON -DBEATGLASS_ENABLE_MACOS_NATIVE_OVERLAY_TESTS=ON`, build `macos_native_overlay_tests`, and run `ctest --test-dir <build-dir> -C Debug -R '^macos_native_overlay$' --output-on-failure` in the logged-in Cocoa session. That opt-in check inspects the AppKit window level, Spaces hints, input flag and unlock on control-window hide. It does not inject pointer events or establish visibility over fullscreen applications; those interactive scenarios remain separately required. The native test has not yet been compiled or run.

A combined serial Windows build with `-DBEATGLASS_BUILD_WINDOWS_OVERLAY_TESTS=ON -DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON`, `cmake --build C:/Users/zachf/.codex/builds/beatglass-macos-overlay --config Debug --target windows_overlay_tests macos_overlay_tests --parallel 1`, and `ctest --test-dir C:/Users/zachf/.codex/builds/beatglass-macos-overlay -C Debug -R '^(windows_overlay|macos_overlay_)' --output-on-failure` passed 3/3. This includes the existing Windows overlay test, the fake-native Qt controller test with real QML attachment, and the offscreen QML navigation test. Native pointer-injection cases in the Windows test were not enabled.

Display selection and compact/medium/large size presets were added after the initial overlay checks. The offscreen controller and QML CTest cases passed 2/2 with the same serial build command, including invalid display/size inputs and bridge calls. The screen list uses session-local Qt order; the feature has not been checked with multiple physical displays or changing DPI.

Geometry and opacity persistence are covered with an isolated temporary QSettings file, including an off-screen saved position, a zero saved opacity (restored at the safe 0.2 minimum), and a check that click-through never persists. The final serial controller build and the two offscreen controller/QML CTest cases passed; the API inventory and warnings-as-errors documentation build passed. These checks do not use or overwrite the application user settings and do not establish AppKit behavior.

After reconciling with merged capture PR #36 at `a609349`, the serial Windows foundation/production-target suite passed 50/50 and the combined overlay CTest cases passed 3/3. The API inventory and warnings-as-errors documentation build passed. Mac-native overlay and pointer behavior remain untested.

The first hosted macOS overlay build compiled the AppKit source but failed to link because Qt 6.8.3's `WrapOpenGL` adds an AGL framework link even when Xcode 26's SDK no longer provides it. [Apple's macOS 26 release notes](https://developer.apple.com/documentation/macos-release-notes/macos-26-release-notes) state that the 64-bit AGL framework was removed and its symbols were inert. The CMake overlay path now drops only the AGL entry when the selected SDK has no AGL library and retains OpenGL; the hosted macOS overlay job is the regression check for that adjustment. A new run is pending, so this is not yet evidence that the workaround works on Zach's Mac.

The subsequent hosted macOS overlay job passed in GitHub Actions run 42384709900 after the AGL adjustment, compiling the AppKit source and running offscreen tests. That is a hosted build check, not a real desktop qualification. An earlier documentation CI run also caught a missing blank line introduced during merge reconciliation; the reference was corrected and the warnings-as-errors build is rerun below.

On 2026-11-08, the native AppKit overlay test was configured and run on
`zachbook-pro` (macOS 26.7.2, Intel MacBookPro16,1, Xcode 26.2, Clang 23.1.5,
CMake 4.4.5). Because the desktop target currently requires the aubio-backed
slice, this isolated native check used `BEATGLASS_BUILD_DESKTOP=OFF`,
`BEATGLASS_BUILD_SLICE=OFF`,
`BEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON`, and
`BEATGLASS_ENABLE_MACOS_NATIVE_OVERLAY_TESTS=ON`. The command built
`macos_native_overlay_tests` successfully and CTest passed `1/1`
(`macos_native_overlay`). The linker emitted a non-fatal `/lib` search-path
warning. This establishes a native AppKit test result for the isolated overlay
controller only; production desktop, audio capture, permission, pointer,
fullscreen, Spaces, and multi-display qualification remain open.
