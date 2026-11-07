# Beatglass

Beatglass is planned as a local desktop music analysis application with a library for analyzing files and a DJ mode that monitors a selected system output and displays live musical estimates. Neither mode is implemented yet. A preliminary file-tempo CLI and metadata-only shell are available when built with optional dependencies.

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

```sh
sudo apt-get install libaubio-dev libsndfile1-dev qt6-declarative-dev libgl1-mesa-dev qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript
cmake -S . -B build -DBUILD_TESTING=ON -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBEATGLASS_TEST_V1_CODECS=OFF
cmake --build build
ctest --test-dir build --output-on-failure
./build/beatglass-cli /path/to/file.wav
./build/beatglass-desktop
```

For Windows the experimental CLI was built with MSVC and vcpkg's `aubio[core]` and `libsndfile[core]` packages by passing `-DCMAKE_PREFIX_PATH=<vcpkg>/installed/x64-windows` and the options above; use matching Debug or Release configuration for build and CTest. Ubuntu 22.04 ships a libsndfile older than the 1.1.0 MP3 support baseline; this example skips the v1 codec integration test and is not format qualification. This system-installed dependency setup is not pinned and these are development instructions, not a release recipe; the experimental manifest pins a separate configuration below. The CLI accepts one file, prints an aubio estimate or unknown, and never modifies the input. The shell only displays basic decoded metadata; it does not launch analysis. Actual support depends on decoder build options. No GUI, capture or overlay has been qualified on any platform.

## API documentation

The provisional library and application-private C++ interfaces are documented with Hawkmoth and Sphinx. Build the same Linux container used by CI from the repository root (Docker required):

```sh
docker build -f docs/api/Dockerfile -t beatglass-docs .
docker run --rm -v "$PWD:/work" beatglass-docs sphinx-build -W --keep-going -b html docs/api /work/build/docs/html
```

Open `build/docs/html/index.html`. On Windows PowerShell, use `${PWD}` in the volume argument. Python dependencies, the base image, and direct Clang, Python, pkg-config and Qt packages are pinned. Ubuntu transitive packages still come from live repositories; archived packages may be needed if pinned versions disappear. Pull requests validate the build; pushes to main deploy it through GitHub Pages when Pages is configured for GitHub Actions. The documentation does not establish native platform qualification or API stability.
### Pinned development dependencies (experimental)

`vcpkg.json` pins the registry baseline and requests aubio without its tool dependencies and libsndfile with FLAC/Vorbis/Opus and MPEG features. Set `VCPKG_ROOT` to a vcpkg checkout containing the baseline, then run `cmake --preset vcpkg-core`, `cmake --build --preset vcpkg-core`, and `ctest --preset vcpkg-core`. The optional `vcpkg-desktop` preset enables the manifest's Qt feature. These presets are not yet verified across all platforms or configurations and do not establish a release distribution or codec coverage. Codec integration tests are enabled by default; set `-DBEATGLASS_TEST_V1_CODECS=OFF` only for a development system whose libsndfile lacks those codecs, and do not count that as format qualification. The CLI also accepts `--replay <file>` to print incremental estimates after observed blocks, without real-time pacing. See [contracts and handoff](docs/contracts.md).

The [Phase 0 gate audit](docs/phase0-gate.md) lists remaining prerequisites and proposed module ownership. The gate is closed; the current experimental shell is not the full application.

## Qt 6.8.3 prebuilt desktop preset

With a clean vcpkg root and the selected Qt 6.8.3 SDK, set `VCPKG_ROOT` to the pinned vcpkg checkout and `BEATGLASS_QT_ROOT` to the SDK's compiler-specific directory (the directory containing `lib/cmake/Qt6`). From a fresh build directory run:

```sh
cmake --preset prebuilt-qt-desktop
cmake --build --preset prebuilt-qt-desktop
ctest --preset prebuilt-qt-desktop
```

On Windows with Visual Studio, add `-G "Visual Studio 17 2022" -A x64` to the configure command. The SDK must match the compiler and target architecture. The preset enforces version 6.8.3 and uses the core vcpkg manifest; do not enable its `desktop` feature or point it at an install tree containing a different Qt version. The source-building `vcpkg-desktop` preset remains experimental. This is a development proposal, not a finished cross-platform package or fully hashed artifact lock; acquisition, platform and license caveats are in [decisions](docs/decisions.md).
