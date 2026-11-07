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

The default builds queue tests only. To build the experimental CLI, install aubio and libsndfile development packages and configure with `-DBEATGLASS_BUILD_SLICE=ON`. Add `-DBEATGLASS_BUILD_DESKTOP=ON` and Qt 6 Quick development and QML runtime modules for the metadata shell. For example on Ubuntu 22.04:

```sh
sudo apt-get install libaubio-dev libsndfile1-dev qt6-declarative-dev libgl1-mesa-dev qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript
cmake -S . -B build -DBUILD_TESTING=ON -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_DESKTOP=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/beatglass-cli /path/to/file.wav
./build/beatglass-desktop
```

For Windows the experimental CLI was built with MSVC and vcpkg's `aubio[core]` and `libsndfile[core]` packages by passing `-DCMAKE_PREFIX_PATH=<vcpkg>/installed/x64-windows` and the options above; use matching Debug or Release configuration for build and CTest. These dependencies are not yet pinned and these are development instructions, not a release recipe. The CLI accepts one file, prints an aubio estimate or unknown, and never modifies the input. The shell only displays basic decoded metadata; it does not launch analysis. Actual support depends on decoder build options. No GUI, capture or overlay has been qualified on any platform.
