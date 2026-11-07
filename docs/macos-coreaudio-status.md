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

After an include-only cleanup, the build and `ctest --test-dir /tmp/beatglass-coreaudio-root-build --output-on-failure -R "Core Audio"` passed again. The linker reported an existing `/lib` search-path warning and a duplicate core-library warning. The test allows an empty device list and does not simulate hotplug or errors; it does not establish actual capture, desktop interaction or overlay qualification.
Follow-up discovery boundary and retry regression checks on the same Mac: the root build succeeded and both new device-list tests passed, but the full suite was 24/25 because the existing concurrent queue stress test exceeded its 30-second timeout during a period of SSH unresponsiveness. The warnings-as-errors documentation build passed. The suite therefore needs a clean rerun.

## Unqualified process-tap prototype

The separate tap branch adds an internal `OutputTap` prototype, not yet connected to the desktop or `AudioSource`. It selects stream zero of the chosen output UID, requests an unmuted private tap, creates a private aggregate input, and copies interleaved float32 PCM into a bounded SPSC FIFO. Other formats fail closed; no conversion is attempted. The control owner must poll status and invalidate published evidence on interruption. Polling detects device liveness, replacement of the first stream, and tap format changes; it cannot prove immediate notification of all route or permission changes. The caller must provide a fresh process-unique nonzero epoch on each restart. A valid host timestamp is retained as raw host ticks; conversion, latency and uncertainty must be resolved before publication as a shared clock anchor. FIFO overflow drops complete blocks and marks the next accepted block discontinuous.

Core Audio's permission prompt is driven by the OS; a successful start does not establish that permission was granted or that the stream contains real audio. The application bundle needs `NSAudioCaptureUsageDescription`; no packaging/signing or denial/revocation recovery is implemented here. A failed IOProc stop/destroy deliberately retains its context instead of freeing memory still potentially used by the audio thread. Native build, actual output audio, permission flow, lifecycle changes, GUI and overlay remain untested; do not enable this prototype as a product source yet.

Contract review of draft PR #28: the adapter needs an explicit way for the process coordinator to allocate and supply the epoch before any callback PCM can be accepted, and a control-event delivery method/ordering contract that says when format plus optional clock anchor are installed relative to PCM. `AudioSource::start` as proposed exposes neither in its signature. On macOS `AudioTimeStamp` host time can be invalid; when valid it needs checked conversion from host ticks and an identified clock domain, with latency/uncertainty unknown unless measured. A Core Audio start success or silent data alone cannot prove permission; report denial only when the OS gives a reliable error, and leave other cases unclassified. Selected-device first-stream scope must be exposed to users rather than implying all endpoint streams are captured. Stop failure semantics need an explicit strategy because freeing a live IOProc context violates callback quiescence.

Portable FIFO validation on the Windows host, based on `7811af2` plus the tap prototype: `cmake -S . -B C:/Users/zachf/.codex/builds/beatglass-macos-fifo -G Ninja -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTING=ON`, `cmake --build C:/Users/zachf/.codex/builds/beatglass-macos-fifo --parallel 4`, and `ctest --test-dir C:/Users/zachf/.codex/builds/beatglass-macos-fifo --output-on-failure` passed 25/25 tests. The Linux-container documentation build from README with `-W --keep-going` passed. These do not compile the Objective-C++ source or validate Core Audio. Repeated SSH connection attempts to `zachbook-pro` timed out during banner exchange, so Mac build and all real capture checks remain pending.

After reconciling with main through `9488384`, the same Windows build and CTest commands passed 28/28; `python docs/api/check_headers.py` and `python -m unittest discover -s tests/unit -p check_headers_test.py` passed. These checks still do not compile the macOS tap implementation.
