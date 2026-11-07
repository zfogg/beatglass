# Progress and handoff

Phase 0 is **not complete**. No feature work should start until its exit gate is met.

- [x] Initial repository guidance and README.
- [x] Initial CMake, pinned Catch2, CTest and three-platform CI configuration.
- [x] Draft audio, result and job contracts; bounded SPSC queue with unit tests.
- [ ] Verify dependency builds, licenses, architectures and minimum OS versions on all platforms.
- [ ] Finalize source lifecycle, estimator, persistence and presentation contracts; define normalization and channel reduction.
- [ ] Real WAV decoder, incremental aubio tempo CLI, metadata/status Qt shell and replay integration test.
- [ ] Build presets, formatting, dependency pins and tested build/run instructions.
- [ ] Publish ownership and handoff after the foundation exit gate.

No capture backend, analysis engine, database, UI, or product executable exists. The schemas are preliminary and are not yet a stable handoff. The queue is a starting point, not proof of real-time behavior on any device. A consumer must treat a discontinuity as a reset, and can drain stale queued blocks before reacquiring; epoch changes also reset state. Stop and join producer/consumer before destruction.

Native desktop/audio qualification remains pending on Linux, Windows and macOS. The reachable macOS host is Intel x86_64, Darwin 25.6.0; SSH alone cannot qualify its desktop or audio. Linux and macOS CI results, UI, real capture, overlay, and actual music have not been verified in this increment.
