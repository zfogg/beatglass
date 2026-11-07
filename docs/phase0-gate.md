# Phase 0 gate audit and ownership handoff

Status: **closed**. This audit records concrete evidence and outstanding exit work; it is not authorization to start parallel feature work. The integration owner controls this audit, shared headers, schema, dependency definitions, root CMake and CI.

| Exit item | Current evidence | Required next action / owner |
| --- | --- | --- |
| Repository guidance and issue plan | README and AGENTS exist; this checklist and decisions are maintained. | Integration owner keeps them current with each merged increment. |
| Reproducible build and dependencies | vcpkg baseline, core and desktop presets exist. Core Windows manifest passed; Ubuntu WSL and Intel Mac builds used system/isolated dependencies. | Integration owner verifies pinned desktop on Windows, Linux and macOS, supported architectures/OS minimums, CI parity and exact fresh build commands. The Windows desktop vcpkg configure failed installing ICU 78: the generated relative `install-sh` path was not found even though the source file exists; inspect the local install log before selecting a fix. |
| Licensing and decoder | aubio and libsndfile versions/licenses recorded; generated PCM WAV works. | Integration owner resolves compatible distribution path including repository license, notices and transitive dependencies; verifies FLAC, MP3, AIFF and investigates AAC/M4A and codec licensing per platform. |
| Shared contracts | Borrowed PCM, bounded queue, continuity, frame-clock mapping, immutable bounded-count snapshot publication, preliminary strategies, job/store and presentation boundaries exist. | Integration owner finalizes sample-clock anchoring/validity, loss signaling, bounded publication, source lifecycle/errors/cancellation, storage transactions and Qt presentation ownership; validate interfaces against real adapters. |
| File CLI and shell | WAV decode and aubio incrementally produce estimates; a Qt shell displays actual metadata. Windows CLI and Qt 6.8.3 desktop, WSL and Intel Mac shell builds/tests recorded; Windows startup was noninteractive. | Integration owner verifies pinned Windows desktop deployment and executes interactive shell file selection on real desktops; resolve synchronous shutdown behavior on blocked filesystem reads. Confirm stable run commands. |
| Replay and tests | Generated WAV, silence, stereo, prefix causality and tracker reset coverage run under CTest. | Integration owner adds exercised end-to-end source discontinuity, seek, changing sample rate and stale publication scenarios, then ensures CI tests the same pinned dependencies across platforms. |
| Handoff | Provisional module ownership and APIs documented in contracts.md. | Integration owner publishes stable reviewed interfaces and precise module branches/worktrees only after all gate rows pass. |

The issue's native Linux, Windows and macOS library/DJ/overlay qualification is a separate v1 requirement. WSL, CI and SSH/offscreen checks do not satisfy it, and no supported product platform is claimed. Native qualification remains pending even if this Phase 0 gate later passes.

## Intended ownership once the gate passes

- Integration owner: public contracts, result schema, database schema, CMake/dependencies and CI; coordinate versioned changes and perform continuous integration.
- DSP/CLI owner: `lib/core`, `lib/capture/file`, `src/cli`, associated tests; share decoder ownership contract before changes.
- Library owner: `lib/library`, associated tests; propose schema changes to integration owner.
- Shared UI owner: `src/desktop/qml` and presentation models, associated tests; coordinate public presentation interface.
- Platform owners: `lib/capture/pipewire`, `lib/capture/coreaudio`, `lib/capture/wasapi`, plus assigned platform-window files and tests. Each works in an isolated branch/worktree; shared headers and build configuration require integration coordination.

No module handoff makes a stub a product feature. Every workstream must run meaningful tests, report exact commands and native limitations, preserve user files, and respect callback and thread constraints in AGENTS.md.
