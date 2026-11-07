# Progress and handoff

Phase 0 is **not complete**. No feature work should start until its exit gate is met.

- [x] Initial repository guidance and README.
- [x] Initial CMake, pinned Catch2, CTest and three-platform CI configuration.
- [x] Draft audio, result and job contracts; bounded SPSC queue with unit tests.
- [ ] Verify dependency builds, licenses, architectures and minimum OS versions on all platforms.
- [ ] Finalize source lifecycle, estimator, persistence and presentation contracts; define normalization and channel reduction.
- [ ] Complete and qualify the WAV/aubio CLI and Qt shell: the initial file slice exists, but incremental source contracts, replay and cross-platform dependency pins remain open.
- [ ] Build presets, formatting, dependency pins and tested build/run instructions.
- [ ] Publish ownership and handoff after the foundation exit gate.

A preliminary file CLI and metadata-only Qt shell exist; there is no capture backend, key/chord engine, database, or complete product UI. The schemas are preliminary and are not yet a stable handoff. The queue is a starting point, not proof of real-time behavior on any device. A consumer must treat a discontinuity as a reset, and can drain stale queued blocks before reacquiring; epoch changes also reset state. Stop and join producer/consumer before destruction.

Native desktop/audio qualification remains pending on Linux, Windows and macOS. The reachable macOS host is Intel x86_64, Darwin 25.6.0; SSH alone cannot qualify its desktop or audio. Linux and macOS CI results, UI, real capture, overlay, and actual music have not been verified in this increment.

## Validation of commit 4997668

- Windows, GCC 15.2.0: `cmake -S . -B build -DBUILD_TESTING=ON`, `cmake --build build -j 6`, `ctest --test-dir build --output-on-failure`: 3/3 tests passed.
- `ssh zachbook-pro`, Intel x86_64 Darwin 25.6.0, Clang 23.1.5: archived commit to `/tmp/beatglass-phase0-c429`, then `cmake -S /tmp/beatglass-phase0-c429 -B /tmp/beatglass-phase0-c429-build -G Ninja -DBUILD_TESTING=ON`, `cmake --build /tmp/beatglass-phase0-c429-build -j 4`, `ctest --test-dir /tmp/beatglass-phase0-c429-build --output-on-failure`: 3/3 passed; linker warned that `/lib` search path was absent. This is not GUI or audio qualification.

## Experimental slice validation

- Windows x64 MSVC 19.44, vcpkg aubio core and libsndfile core: `cmake -S . -B build-slice -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Users/zachf/scoop/apps/vcpkg/current/installed/x64-windows`, `cmake --build build-slice --config Debug -j 6`, `ctest --test-dir build-slice -C Debug --output-on-failure`: 6/6 passed. Qt was not built on Windows.
- Ubuntu 22.04 under WSL2 x86_64 GCC 11.4: `cmake -S /mnt/c/Users/zachf/.codex/worktrees/c429/beatglass -B /tmp/beatglass-slice -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBUILD_TESTING=ON`, `cmake --build /tmp/beatglass-slice -j 6`, `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 6/6 passed. Offscreen Qt process started and remained running for three seconds; no interactive file selection was exercised. WSL is not native Linux qualification.
- macOS slice and Qt shell not built; an existing Homebrew upgrade holds the package lock. GUI/audio/overlay qualification on actual Linux, Windows and macOS remains pending. Synthetic clicks do not demonstrate accuracy on music.

## Metadata worker follow-up

Metadata file opening runs in the global Qt thread pool rather than the GUI thread. At most one read runs per model, with only the newest pending selection retained; obsolete results are not displayed. Destruction invalidates the delivery target without waiting for the read. Underlying blocking filesystem calls are not cancellable, and Qt may wait for global pool tasks during process shutdown; an indefinitely stalled filesystem can therefore still delay shutdown.

Ubuntu 22.04 WSL2: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure` passed 7/7. The Qt test checks GUI event-loop responsiveness during a blocked reader, coalescing and stale results, exception reporting, and destruction during a blocked read. Windows Qt is not installed in the implementing environment, so the new Windows desktop deployment path and Qt tests are unverified. Previous PR CI passed on all three platform test jobs and Linux slice; the new commit's checks remain to be observed.

## Incremental tempo follow-up

`TempoTracker` is a preliminary worker-owned incremental mono adapter. The file analyzer now uses it; the current API does not yet expose beat timestamps, scores or the full snapshot state. Windows MSVC Debug: `cmake --build build-slice --config Debug -j 6` and `ctest --test-dir build-slice -C Debug --output-on-failure`: 8/8 passed. Ubuntu 22.04 WSL2 with desktop enabled: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 9/9 passed. Chunk-size equivalence, gaps, epochs and invalid sample rate are tested. Mac and actual device replay remain untested.

## Replay, dependency and contract increment

`--replay` feeds observed decoded blocks through the same incremental tracker without pacing; generated-audio integration verifies acquisition, prefix causality and final agreement with normal file analysis. Provisional ports and [handoff notes](contracts.md) describe integration boundaries, but implementations, sample-clock mapping, SQLite transactions, presentation and real capture remain open. A vcpkg manifest pins a baseline and optional Qt feature; cross-platform manifest/desktop verification and distribution decisions are not complete. Ubuntu 22.04 WSL2, `cmake --build /tmp/beatglass-slice -j 6` then `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 10/10 passed. This used distribution libraries rather than the pinned manifest and does not qualify native Linux.

Windows x64 MSVC Debug, development dependencies: `cmake --build build-slice --config Debug -j 4`, `ctest --test-dir build-slice -C Debug --output-on-failure`: 9/9 passed. A separate pinned manifest build passed 9/9: `cmake -S . -B build-manifest -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=C:/Users/zachf/scoop/apps/vcpkg/current/scripts/buildsystems/vcpkg.cmake`, `cmake --build build-manifest --config Debug -j 6`, `ctest --test-dir build-manifest -C Debug --output-on-failure`. The optional Qt feature and presets were not run. WSL distribution dependency test was rerun after the prefix-causality fixture: 10/10 passed.
