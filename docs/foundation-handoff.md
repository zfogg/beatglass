# Foundation handoff: ownership and integration points

These are concrete starting points for integration-owner review, not a claim that all interfaces are stable or every feature is implemented. The owner should resolve the open choices below and name the checked-out base commit before assigning worktrees. See the [Phase 0 audit](phase0-gate.md) for the actual exit criteria.

| Workstream | Owned implementation | Integration point and constraints |
| --- | --- | --- |
| Integration | Root CMake/presets, CI, dependencies, shared `include/beatglass/` headers and schema coordination | Reviews all shared contract/schema changes, merges small increments, runs integrated checks and records build identity. |
| DSP and CLI | `lib/core/`, `lib/capture/file/`, `src/cli/`, matching tests | `AudioBlock`, `StreamContinuity`, estimator/result types and `FileReader`; the file reader owns a synchronous decoder and returns caller-buffer-backed samples. Coordinate decoder ownership with library. Do not promise key/chord output from the existing tempo-only slice. |
| Library backend | `lib/library/`, matching tests | `AnalysisStore`, `FileVersion`, `AnalysisIdentity`, and job types are the starting boundary; worker owns database work. Coordinate schema/identity and actual transaction semantics with integration. Never write user tags or delete source files without explicit action. |
| Library UI and shared DJ UI | `src/desktop/` and QML, matching tests, with native window files assigned separately | Receive immutable `PublishedSnapshot` values and translate to Qt-owned models on the GUI thread. Coordinate stale-result invalidation, update delivery/order, lifetime and GUI shutdown with integration; `PresentationAdapter` is provisional, not an implemented UI model. |
| Linux platform | `lib/capture/pipewire/`, assigned `src/desktop/platform/` files, matching tests | `AudioSource`, `SourceLifecycle`, `AudioQueue`, audio/clock types; implement sink-monitor capture, document callback and device lifecycle, and propose platform-window changes. |
| macOS platform | `lib/capture/coreaudio/`, assigned platform files, matching tests | Same source boundary; implement Core Audio taps and permission/status lifecycle. SSH builds do not qualify the interactive desktop. |
| Windows platform | `lib/capture/wasapi/`, assigned platform files, matching tests | Same source boundary; implement WASAPI output loopback and endpoint lifecycle. |

Audio callbacks may only copy into preallocated bounded storage and record timing/status. `AudioBlock` samples are borrowed; workers cannot retain callback memory. The consumer owns continuity/tracking and clears stale publication on interruption. Each new source epoch must identify its own format/timeline; anchor validity, clock accuracy and drift must be specified by the adapter. The existing `SourceLifecycle` is a control-thread helper, not a capture backend or cross-thread notification queue. No Qt, DSP, database, allocation, ordinary logging or blocking lock belongs in the callback.

## Decisions to close during integration review

1. Define typed source enumeration/start failures and control-event ordering, bounded transfer and epoch allocation across source object recreation; preserve stop/quiescence and callback rules.
2. Decide how cancellation and typed cache lookup/publish outcomes cross the worker-owned `AnalysisStore` boundary, including file-version recheck and the cancellation-versus-commit point. SQLite schema, migrations and implementation belong to the library workstream.
3. Define how the UI receives ordered snapshot and invalidation notifications and owns retained values. The publisher does not automatically revoke copies held by GUI code. PR #16 proposes one ordering helper for review; integration must choose an actual delivery contract before relying on it.
4. Fix the estimator adapter/result schema details needed by each initial workstream, including beat/chord boundary semantics and unknown/confidence meaning. Distinguish current implementations from interface targets.
5. Resolve dependency/distribution choices, including Essentia and codec license obligations, and record the selected platform builds. Until then, no release-distribution compatibility is asserted.

## Starting commands and evidence

Use the repository README and CMake presets for current configure/build/CTest commands; `docs/progress.md` records exact environment-specific commands and results. Run tests that exercise actual production targets, plus relevant existing tests. Each handoff reports changed modules and APIs, commands, results, device/OS evidence and unresolved limitations. Native audio, desktop and overlay qualification is required before v1 completion, not before assigning these Phase 1 modules.
