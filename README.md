# Beatglass

Beatglass is planned as a local desktop music analysis application with a library for analyzing files and a DJ mode that monitors a selected system output and displays live musical estimates. Neither mode is implemented yet.

The intended stack is C++20, CMake, Qt 6 Quick, SQLite, aubio and Essentia, with PipeWire, Core Audio taps and WASAPI loopback for Linux, macOS and Windows respectively. Dependency availability, versions and licenses must be verified before selection. No account or network service should be required at runtime.

## Current status and prerequisites

This repository is at the initial groundwork stage. There is no buildable application yet. The preliminary shared contracts and queue have a CMake/CTest test target; see [progress](docs/progress.md) and [decisions](docs/decisions.md). A C++20 compiler, CMake, Qt 6 and platform-specific audio development dependencies are anticipated; exact versions, installation instructions and reproducible build, run and test commands will be added as the foundation is verified. All three platforms are currently unsupported.

Production implementation belongs in `src/` for applications and `lib/` for reusable libraries, with library headers in `include/beatglass/`, tests and fixtures in `tests/`, and documentation in `docs/`. See issue #1 for the complete specification, acceptance criteria and phased execution plan.

## Build and test the groundwork

Install a C++20 compiler, CMake 3.24 or newer, and Git/network access for the pinned Catch2 download. From the repository root:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

These commands build tests only; there is no CLI or desktop application to run yet. Native platform prerequisites and application commands remain to be established.
