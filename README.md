# Beatglass

Beatglass is planned as a local desktop music analysis application with a library for analyzing files and a DJ mode that monitors a selected system output and displays live musical estimates. Neither mode is complete yet. A preliminary file-tempo CLI and desktop shell with Library and DJ navigation are available when built with optional dependencies. The Library page can browse folders, inspect metadata and play or seek a selected file; library management remains experimental. The draft Windows live tempo worker remains disconnected from the product UI pending capture freshness and loss checks.

The intended stack is C++20, CMake, Qt 6 Quick, SQLite, aubio and Essentia, with PipeWire, Core Audio taps and WASAPI loopback for Linux, macOS and Windows respectively. Dependency availability, versions and licenses must be verified before selection. No account or network service should be required at runtime.

## Current status and prerequisites

This repository is at the initial groundwork stage. The full application is not implemented. The preliminary shared contracts and queue have a CMake/CTest test target; see [progress](docs/progress.md) and [decisions](docs/decisions.md). A C++20 compiler, CMake, Qt 6 and platform-specific audio development dependencies are anticipated; exact versions, installation instructions and reproducible build, run and test commands will be added as the foundation is verified. All three platforms are currently unsupported.

Production implementation belongs in `src/` for applications and `lib/` for reusable libraries, with library headers in `include/beatglass/`, tests and fixtures in `tests/`, and documentation in `docs/`. See issue #1 for the complete specification, acceptance criteria and phased execution plan.

## Build and test the groundwork

Install a C++20 compiler, CMake 3.24 or newer, and Git/network access for the pinned Catch2 download. From the repository root:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The default builds the dependency-light foundation tests. To build the experimental CLI, install aubio and libsndfile development packages and configure with `-DBEATGLASS_BUILD_SLICE=ON`. Add `-DBEATGLASS_BUILD_DESKTOP=ON` and Qt 6 Quick development and QML runtime modules for the metadata shell. For example on Ubuntu 22.04:

The draft SQLite collection bridge is separately enabled by `-DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON -DBEATGLASS_BUILD_LIBRARY_FINGERPRINT=ON` together with the desktop and slice options. Setting `BEATGLASS_EXPERIMENTAL_LIBRARY_DB` to a development database path adds an experimental, bounded indexed-file page to the desktop. When the private composite replay is also built, the Library can explicitly analyze a selected file without caching and show provisional rolling key and local chord windows; without a database path it uses an in-memory store. These estimates are not a validated track key, and source and dependency verification remain provisional. The page does not index files or reuse cached results. SQLite, OpenSSL and the composite runtime deployment are not yet qualified.

```sh
sudo apt-get install libaubio-dev libsndfile1-dev qt6-declarative-dev qt6-multimedia-dev gstreamer1.0-plugins-good libgl1-mesa-dev qml6-module-qtmultimedia qml6-module-qt-labs-folderlistmodel qml6-module-qttest qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml qml6-module-qtqml-models qml6-module-qtqml-workerscript
cmake -S . -B build -DBUILD_TESTING=ON -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBEATGLASS_TEST_V1_CODECS=OFF
cmake --build build
ctest --test-dir build --output-on-failure
./build/beatglass-cli /path/to/file.wav
./build/beatglass-desktop
```

For Windows the experimental CLI was built with MSVC and vcpkg's `aubio[core]` and `libsndfile[core]` packages by passing `-DCMAKE_PREFIX_PATH=<vcpkg>/installed/x64-windows` and the options above; use matching Debug or Release configuration for build and CTest. Ubuntu 22.04 ships a libsndfile older than the 1.1.0 MP3 support baseline; this example skips the v1 codec integration test and is not format qualification. This system-installed dependency setup is not pinned and these are development instructions, not a release recipe; the experimental manifest pins a separate configuration below. The CLI accepts one file, prints an aubio estimate or unknown, and never modifies the input. The Library page displays basic decoded metadata and provides file playback and seeking. Explicit provisional uncached analysis is available only in the private composite-enabled configuration described above. Playback format availability depends on the Qt Multimedia backend. On Windows with the default FFmpeg backend, an output reporting an unusable channel layout is blocked with a warning; choose another playback output if available. This does not change the system default. The experimental Windows DJ page can show a controllable overlay; its live output monitoring remains disconnected pending capture freshness and loss checks; live monitoring on other platforms and live key/chord/beat-phase presentation remain unavailable. The capture and overlay adapters are draft integrations, not qualified platform support. Actual support depends on decoder build options. No GUI, capture or overlay has been qualified on any platform.

On Linux, `-DBEATGLASS_BUILD_PIPEWIRE=ON` builds experimental PipeWire sink discovery and its CTest coverage; install the PipeWire development package first (for Ubuntu, `libpipewire-0.3-dev`). This does not yet provide capture or a DJ workflow. See [Linux platform status](docs/linux-platform-increment.md).

On macOS the DJ page also offers an experimental overlay; live output monitoring is not yet connected there. Its native behavior remains unqualified; see [macOS overlay status](docs/macos-overlay.md).

The desktop has a provisional per-user relaunch handoff and tray/menu actions for restoring controls, unlocking or hiding the overlay, and quitting. The controls remain visible when the overlay opens until a reachable recovery route has been verified on the active platform; tray visibility alone is not sufficient evidence. Native tray, Dock/menu-bar, compositor and relaunch behavior remain unqualified.

For automated launches, `BEATGLASS_NONINTERACTIVE=1` reports startup failures to stderr instead of opening a dialog. Qt's offscreen and minimal platforms also use stderr automatically. The production instance-lock directory remains per-user and cannot be redirected by this setting.

## API documentation

The provisional library and private `src/` and `lib/` C++ interfaces are documented with Hawkmoth and Sphinx; indexing a private header does not install it. Check header coverage with `python docs/api/check_headers.py` and `python tests/unit/check_headers_test.py`. Build the same Linux container used by CI from the repository root (Docker required):

```sh
docker build -f docs/api/Dockerfile -t beatglass-docs .
docker run --rm -v "$PWD:/work" beatglass-docs sphinx-build -W --keep-going -b html docs/api /work/build/docs/html
```

Open `build/docs/html/index.html`. On Windows PowerShell, use `${PWD}` in the volume argument. Python dependencies, the base image, and direct Clang, Python, pkg-config and Qt packages are pinned. Ubuntu transitive packages still come from live repositories; archived packages may be needed if pinned versions disappear. Pull requests validate the build; pushes to main deploy it through GitHub Pages when Pages is configured for GitHub Actions. The documentation does not establish native platform qualification or API stability.
### Pinned development dependencies (experimental)

`vcpkg.json` pins the registry baseline and requests aubio without its tool dependencies and libsndfile with FLAC/Vorbis/Opus and MPEG features. Set `VCPKG_ROOT` to a vcpkg checkout containing the baseline, then run `cmake --preset vcpkg-core`, `cmake --build --preset vcpkg-core`, and `ctest --preset vcpkg-core`. The optional `vcpkg-desktop` preset enables the manifest's Qt feature. These presets are not yet verified across all platforms or configurations and do not establish a release distribution or codec coverage. Codec integration tests are enabled by default; set `-DBEATGLASS_TEST_V1_CODECS=OFF` only for a development system whose libsndfile lacks those codecs, and do not count that as format qualification. The CLI also accepts `--replay <file>` to print incremental estimates after observed blocks, without real-time pacing. See [contracts and handoff](docs/contracts.md).

The [Phase 0 gate audit](docs/phase0-gate.md) records the coordinator's acceptance for starting Phase 1 and remaining gaps. The current experimental shell is not the full application.

## Qt 6.8.3 prebuilt desktop preset

With a clean vcpkg root and the selected Qt 6.8.3 SDK, set `VCPKG_ROOT` to the pinned vcpkg checkout and `BEATGLASS_QT_ROOT` to the SDK's compiler-specific directory (the directory containing `lib/cmake/Qt6`); install the matching Qt Multimedia module as well. From a fresh build directory run:

```sh
cmake --preset prebuilt-qt-desktop
cmake --build --preset prebuilt-qt-desktop
ctest --preset prebuilt-qt-desktop
```

On Windows with Visual Studio, add `-G "Visual Studio 17 2022" -A x64` to the configure command. The SDK must match the compiler and target architecture. The preset enforces version 6.8.3 and uses the core vcpkg manifest; do not enable its `desktop` feature or point it at an install tree containing a different Qt version. The source-building `vcpkg-desktop` preset remains experimental. This is a development proposal, not a finished cross-platform package or fully hashed artifact lock; acquisition, platform and license caveats are in [decisions](docs/decisions.md).

File replay uses the proposed worker-owned `CausalTempoReplay` block interface. It accepts arbitrary interleaved chunk sizes, buffers partial 512-frame hops, and reports accepted and analyzed exclusive boundaries separately. An incomplete final hop is not analyzed. The private library snapshot jobs can feed this replay; live capture is not connected.

The optional [tonal worker experiment](docs/tonal-experiment.md) requires Essentia and feeds only the explicitly enabled provisional composite analysis path; it is not a verified or cacheable product result.

The experimental worker-only aubio beat tracker is used by the combined replay; see [progress](docs/progress.md) for its timing and qualification limits.

## Experimental combined file and block replay

With `BEATGLASS_BUILD_SLICE=ON`, `BEATGLASS_BUILD_TONAL_SPIKE=ON`, and `BEATGLASS_BUILD_MUSIC_REPLAY=ON`, the optional `beatglass-music-cli <file>` emits JSON Lines from actual aubio and Essentia analysis. `beat` records carry a source epoch, event frame and exclusive observed boundary. `tonal` records carry a half-open chord-analysis window, an optional earlier `rolling_key_from` boundary for the rolling key evidence, and `rolling_key`/`local_chord` labels or null. Beat strength is null because this tracker does not provide one; tonal strengths are uncalibrated and null when unavailable. These overlapping windows are not chord segments, and a rolling key is not a track-level key. `state` records report lifecycle changes; consumers should clear prior labels on silence or reacquisition. The final `result` records tempo, latest tonal labels and observed/analyzed boundaries; a failure can leave a partial stream and exits nonzero. The source file is not modified. This is a fast, causal replay without playback pacing, a calibrated model or release qualification.

The same `CausalMusicReplay` consumes worker-delivered live `AudioBlock`s; its explicit interruption operation clears internal evidence and requires a fresh coordinator-issued epoch. The caller must also deliver revisioned clears to UI consumers. A provisional library snapshot worker feeds the collector without cache reuse; no capture backend or desktop presentation is connected. Generated synthetic tests do not establish real-music accuracy, latency, platform support, or legal clearance for distributing Essentia.


The [Essentia dependency spike](docs/essentia-spike.md) records research builds, licensing considerations and the proposed tonal architecture; the combined CLI remains experimental.

## Provisional library storage increment

`BEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON` enables private SQLite and job-executor primitives; the optional experimental desktop bridge also requires the fingerprint and desktop configurations described above. SQLite is currently found from the host and is not pinned. On WSL Ubuntu 22.04, install `libsqlite3-dev`, then configure and test as follows (the Catch2 override is optional when network download is available):

```sh
cmake -S . -B /tmp/beatglass-library-phase1 -DBUILD_TESTING=ON -DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src
cmake --build /tmp/beatglass-library-phase1 -j 3
ctest --test-dir /tmp/beatglass-library-phase1 --output-on-failure
```

This is an isolated review increment, not a product-ready library backend. Snapshot verification, shared replay and UI integration, full result schema, database location and dependency pinning remain open. Do not use the prototype for user data. It does not write audio or tags.

The follow-up prototype can be built with `-DBEATGLASS_BUILD_LIBRARY_FINGERPRINT=ON` together with `BEATGLASS_BUILD_LIBRARY_PROTOTYPE` and `BEATGLASS_BUILD_SLICE`; it requires OpenSSL development libraries as well as SQLite, aubio, and libsndfile. It hashes file content, runs the existing tempo replay on cache misses, and exposes cooperative cancellation and progress. Its separate decoder and hash opens leave a concurrent-replacement race, so it is not a user-facing backend; see the [contract proposal](docs/phase1-contract-proposals.md).
File replay and the private snapshot-job experiment use the proposed worker-owned `CausalTempoReplay` block interface. It accepts arbitrary interleaved chunk sizes, buffers partial 512-frame hops, and reports accepted and analyzed exclusive boundaries separately. An incomplete final hop is not analyzed. The indexed snapshot cache path remains gated from the application. Windows live tempo uses a separate worker-owned experimental path; neither it nor the merged live composite worker is connected to the UI.

The experimental CLI also accepts `--json`, alone or with `--replay`. Normal JSON output is one object; replay emits one JSON object per observed block followed by a final result object (JSON Lines). `schema_version` is 1, `observed_through` is an exclusive frame boundary, and `bpm: null` means unknown. The beat count covers the current acquisition. These are preliminary tempo-only outputs: they do not contain key or chord estimates, and replay runs without real-time pacing. Errors are reported on stderr with a nonzero exit status; a replay error can leave a partial JSON Lines stream without a final result.

The merged bounded visualization processor and provisional Windows live composite worker are not yet connected to the desktop. Their results require source/generation handoff and native qualification before presentation.
