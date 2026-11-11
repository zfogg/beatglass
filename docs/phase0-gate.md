# Phase 0 exit gate and handoff

Issue #1 defines the foundation exit gate as **reproducible build commands, real shared contracts, a working CLI slice, a runnable UI shell, and a clear integration point for each team**. The integration owner decides when the merged evidence and contracts meet that gate. This audit does not require completed Phase 1 or v1 features before parallel feature work begins.

Status: **accepted for starting Phase 1** by the coordinator on merged commit `6f927cb`. This is a narrow handoff decision, not a finding that every item in the audit is complete. Shared contract changes, cross-platform dependency and SDK integrity verification, distribution licensing, and native v1 qualification remain open. See [progress](progress.md), [decisions](decisions.md), and [the handoff](foundation-handoff.md) for evidence and ownership.

| Phase 0 exit requirement | Evidence on merged main | Concrete remaining foundation work |
| --- | --- | --- |
| Reproducible build commands | CMake presets and a pinned vcpkg baseline exist; development builds on Windows, WSL and Intel macOS are recorded in progress. CI runs CTest on three hosted OSes. | Integrate and verify the pinned desktop configuration and its actual platform build results; record fresh commands and limitations. The Qt 6.11.1 source build in PR #11 failed in Qt Declarative and that PR was closed as superseded. PR #19 proposes Qt 6.8.3 prebuilt SDKs and records a passing Windows combined build; SDK integrity and other platform builds remain to be reviewed. WSL is not native Linux. |
| Real shared contracts and team integration points | Audio block, bounded queue, sample clock, continuity, tracking, snapshot publisher, file reader, and provisional estimator/source/job/store/presentation interfaces exist. Source lifecycle attempt and epoch rules have tests. | Integration owner reviews and publishes the concrete boundaries and unresolved choices in the handoff, especially error/cancellation reporting, sample-clock anchor validity, job/store outcomes, and presentation delivery. A review decision may assign a specific interface change to the appropriate workstream; it does not require the whole feature to be implemented first. |
| Working CLI slice and deterministic harness | libsndfile reads WAV; aubio consumes incremental audio and CLI emits actual tempo/unknown results. Generated WAV, replay, continuity and source-preservation tests exist. Core/CLI do not need a GUI or audio output device. | Keep the documented commands and tests reproducible against the selected dependencies. The AIFF/FLAC/MP3 dependency configuration and fixtures merged in PR #13; pinned Windows codec tests passed, with WSL codec evidence also recorded in progress. |
| Runnable minimal UI shell | A Qt metadata-only shell runs with actual file metadata; Windows file selection has been exercised using a separate Qt 6.8.3 SDK; the evidence is retained in PR #19. An Intel Mac Qt-enabled build/test has run over SSH; its actual desktop file picker displayed generated WAV metadata through a VNC session, as recorded in progress. | Verify the chosen pinned desktop build and a runnable shell with reproducible commands. This interactive smoke used Homebrew Qt and a temporary application bundle, not the selected pinned SDK or a release package. |
| Dependency/architecture groundwork and ownership | Dependency and platform research, provisional module ownership, and architecture notes exist. | Record a compatible dependency/distribution decision and the known licensing obligations or explicit unresolved decision before locking a distribution; assign worktree ownership and reviewed interface versions for each active workstream. |

## Later-phase requirements, not extra Phase 0 exit conditions

- SQLite migrations/cache, actual job scheduling and transaction recovery are Phase 1 library work. Their **interfaces and ownership** need a usable handoff in Phase 0; their production implementation and acceptance tests do not have to precede the handoff.
- PipeWire, Core Audio and WASAPI live capture, real output-device lifecycle integration, and OS overlay behavior are Phase 1 workstreams. Their integration interfaces and callback/thread rules are foundation requirements; full adapters and device-interruption tests are later work.
- End-to-end library/DJ flows are integrated-alpha work. Real-music measurement, packaging and native Windows/macOS/Linux product, audio and overlay qualification are required before v1 completion. WSL, hosted CI and SSH/headless runs do not replace those checks.

Every workstream uses isolated branches/worktrees and explicit module ownership. The integration owner coordinates shared headers, schemas, CMake, dependencies and CI. Every implementing agent adds meaningful tests, runs relevant checks and records commands, evidence and limitations. Product paths must use actual capture and analysis; no handoff makes a stub a completed feature.

## Handoff decision and remaining work

The coordinator accepted the handoff for starting Phase 1 on `6f927cb`. The working tempo CLI, deterministic replay, minimal interactive Windows and Mac shells, test infrastructure and named team integration points do not establish a usable product. The exact-version SDK remains a development baseline with unresolved archive/installer hashes, Linux/macOS prebuilt builds, and release license/packaging review.

The source adapters, library backend, DSP and UI agents can propose the typed error/event and epoch rules, cache/commit outcomes, estimator semantics and UI delivery details identified in this handoff. The integration owner must approve shared header/schema changes before other work depends on them. Device implementations, SQLite behavior, cross-platform packaging and native v1 audio/overlay qualification remain later-phase requirements.

## Qualification evidence handoff update

The merged qualification tooling now provides a fail-closed handoff path: the status checker requires Windows, macOS and Linux rows; the retained-bundle verifier checks provenance, hashes, canonical paths, required scenario IDs, duplicate/unknown scenarios and one retained artifact per completed scenario; and the report exposes per-platform and aggregate readiness failures. These are preparation and evidence-integrity gates, not native qualification results. Actual desktop, audio and overlay observations on supported hardware remain open, and headless tests, CI, WSL, SSH-only runs and portable fixtures do not satisfy the native gate.

The shared readiness report uses `readiness_report_schema: 1`, `exit_gate_status`, and `exit_gate_reason_codes`. Support language remains experimental until native qualification is complete.

## v2 documentation boundary

The v2 GUI/CLI screen map and implemented-versus-design-only boundary are
maintained in [`v2-status.md`](v2-status.md). The v2 token manifest is a shared
presentation contract, not evidence that the CLI surfaces, native capture, or
overlay behavior are complete. Run the portable checks listed there before
reviewing a v2 documentation change; native platform gates remain governed by
the qualification status below.
# Phase 0 qualification handoff

The local exit-gate workflow is the retained-evidence pipeline documented in `docs/platform-qualification-status.md`: ingest native-script artifacts, generate and validate the conservative report, then run the downstream consumer. A successful portable run proves only that the evidence handoff is internally consistent; it does not satisfy the native Windows, macOS, or Linux desktop/audio/overlay gates. `PENDING` is the expected state until those observations exist.
