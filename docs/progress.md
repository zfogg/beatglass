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

## API documentation

Hawkmoth/Sphinx reference and an Actions-based Pages workflow cover the current provisional headers. The strict documentation build validates parsing, not product functionality or native qualification. Pages publishing requires the workflow to reach main. Reconcile comments and the header index with continuing foundation changes.
## Continuity, clock and normalization increment

`StreamContinuity` detects gaps, discontinuities and epochs, rejects format changes within an epoch, and is used by file replay. `AudioClockAnchor` maps same-epoch frame positions to a monotonic point estimate with overflow checks; drift, anchor validity and backend timing remain open. `reduce_loudest_channel` is now shared by the file path, with anti-phase, malformed and non-finite input tests. The policy is still provisional and may switch channels between blocks.

- Windows x64, MSVC 19.44, pinned vcpkg core, build 43f9bbd: `cmake --build build-manifest --config Debug -j 6` and `ctest --test-dir build-manifest -C Debug --output-on-failure`: 15/15 passed. Windows Qt and the manifest desktop feature remain untested.
- Ubuntu 22.04 WSL2, GCC 11.4, system aubio/libsndfile/Qt, build 43f9bbd: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 16/16 passed. WSL is not native Linux qualification.
- Intel macOS Darwin 25.6.0, Clang 23.1.5, build 43f9bbd, over `ssh zachbook-pro`: aubio upstream commit 152d6819b360c2e7b379ee3f373d444ab3df0895 was built into `/tmp/beatglass-aubio-spike/install` with the local vcpkg port CMake file, `WITH_DEPENDENCIES=OFF` and `BUILD_TOOLS=OFF`; libsndfile 1.2.2 and Qt 6.12.0 were existing Homebrew installations. `cmake -S /tmp/beatglass-phase0-lifecycle-src -B /tmp/beatglass-phase0-lifecycle-build -G Ninja -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="/tmp/beatglass-aubio-spike/install;/usr/local/opt/libsndfile;/usr/local/opt/qtbase;/usr/local/opt/qtdeclarative" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=/usr/local/opt/qtdeclarative`, then `cmake --build /tmp/beatglass-phase0-lifecycle-build -j 4` and `ctest --test-dir /tmp/beatglass-phase0-lifecycle-build --output-on-failure`: 16/16 passed. Linker warned that `/lib` search path was absent. An offscreen shell stayed running for three seconds; no interactive dialog, native overlay or real audio was tested. This is not a pinned Qt manifest build or native GUI/audio qualification.

The repository's proposed handoff still requires actual capture source adapters, clock anchoring and interruption semantics, tested store and presentation implementations, reproducible pinned platform dependencies and native qualification. The foundation gate remains closed.

## Tracking lifecycle and gate audit increment

`TrackingMachine` is worker-owned and guards results with epoch/generation tokens. Replay now clears the tempo and resets the tracker after one second of peak-below-0.0001 signal, and reacquires after audio resumes. This threshold is a provisional tuning choice; it is not a real-music accuracy guarantee. Generated replay tests cover acquisition, silence and resume. [The gate audit](phase0-gate.md) lists remaining concrete work and proposed ownership; the gate stays closed.

- Windows x64, MSVC 19.44, existing vcpkg development libraries: `cmake --build build-slice --config Debug -j 6` and `ctest --test-dir build-slice -C Debug --output-on-failure`: 18/18 passed.
- Ubuntu 22.04 WSL2 with Qt enabled: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 19/19 passed. This is not native Linux qualification.
- Windows pinned desktop: `VCPKG_ROOT=C:/Users/zachf/scoop/apps/vcpkg/current` and `cmake --preset vcpkg-desktop -G "Visual Studio 17 2022" -A x64` failed in ICU 78 during the debug install step: `make` reported `./../src/icu4c-78-34dc699e15.clean/source/install-sh: No such file or directory`, although the source tree contains that file. The log is `build/vcpkg-desktop/vcpkg-manifest-install.log`; configuration/build/test and Qt deployment remain unverified. The build used the shared Scoop vcpkg tree. Intel macOS Darwin 25.6.0 over SSH, build a0befdb with the isolated aubio and Homebrew Qt/libsndfile setup described above: the same CMake configure command, `cmake --build /tmp/beatglass-phase0-lifecycle-build -j 4` and `ctest --test-dir /tmp/beatglass-phase0-lifecycle-build --output-on-failure` passed 19/19; linker warned `/lib` was absent. No interactive GUI or actual audio was tested.

## Immutable snapshot publication increment

`SnapshotPublisher` provides a worker-published latest immutable value, item/text limits, acquisition-token checks and validation of event order/epochs. It is not yet connected to live analysis or the GUI, and caller-owned old snapshots survive invalidation. It is not real-time safe. Windows x64 MSVC: `cmake --build build-slice --config Debug -j 6` and `ctest --test-dir build-slice -C Debug --output-on-failure`: 21/21 passed. Ubuntu 22.04 WSL2, Qt enabled: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 22/22 passed. Intel macOS Darwin 25.6.0 via SSH, build 3dd513c using the isolated aubio and Homebrew Qt/libsndfile setup above: `cmake --build /tmp/beatglass-phase0-lifecycle-build -j 4` and `ctest --test-dir /tmp/beatglass-phase0-lifecycle-build --output-on-failure`: 22/22 passed; linker warned of a duplicate core library and absent `/lib` search path. This is not native GUI/audio qualification.

The strict documentation workflow for the snapshot increment ran `python docs/api/check_headers.py`, `docker build -f docs/api/Dockerfile -t beatglass-docs .`, `docker run --rm --mount type=bind,source=<repository>,target=/work beatglass-docs sphinx-build -E -W --keep-going -b html docs/api /work/build/docs/html`, and `python docs/api/check_rendered.py build/docs/html/index.html`: all passed, covering 17 headers and 50 rendered names.
