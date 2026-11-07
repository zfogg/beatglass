# macOS Core Audio development status

The current platform increment only enumerates output devices using Core Audio's device UID and display name. Enumeration is control-thread work, returns an error separately from an empty list, and skips devices that disappear or whose output streams, UID, or name cannot be queried. Its provisional, non-installed declaration is in `include/beatglass/capture/coreaudio_internal.hpp`; integration remains pending. The list is a point-in-time snapshot, not a hotplug subscription.

Core Audio process taps and aggregate devices, permission handling, stream lifecycle, and overlay integration are not implemented in this increment. Apple's process-tap APIs require macOS 14.2 or later as indicated by the SDK declarations; the app bundle will need `NSAudioCaptureUsageDescription`. Device-specific taps target a stream of the selected output, rather than promising capture of every output. A future adapter must leave output unmuted, handle device and format changes, and deliver only bounded copies from the audio callback. The process-wide epoch allocator, ordered control events, typed errors, and clock-anchor validity need the shared contract decisions in `phase1-contract-proposals.md` before integration.

Native discovery testing is not real output-capture or graphical-overlay qualification. Those scenarios and permission denial/recovery remain pending.

## Development check, 2026-11-07

On `zachbook-pro` (Intel x86_64, macOS 26.7.1 build 25G241, Xcode 26.2, Homebrew Clang 23.1.5), a source copy based on `6f927cb` plus this increment passed the foundation suite, 23/23 tests. Catch2 3.8.1 was fetched and hash checked by the standalone configure first, then reused:

```sh
cmake -S /tmp/beatglass-coreaudio-phase1 -B /tmp/beatglass-coreaudio-root-build -DBUILD_TESTING=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-coreaudio-phase1-build/_deps/catch2-src
cmake --build /tmp/beatglass-coreaudio-root-build --parallel 4
ctest --test-dir /tmp/beatglass-coreaudio-root-build --output-on-failure
```

After an include-only cleanup, the build and `ctest --test-dir /tmp/beatglass-coreaudio-root-build --output-on-failure -R "Core Audio"` passed again. The linker reported an existing `/lib` search-path warning and a duplicate core-library warning. The test allows an empty device list and does not simulate hotplug or errors; it does not establish actual capture, desktop interaction or overlay qualification.\n