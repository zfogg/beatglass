# Progress and handoff

Overlay settings now provide a focused endpoint refresh action that keeps
selection and retry recovery in one workflow. Portable coverage is in
`python tests/unit/check_overlay_endpoint_refresh_action_contract.py`.

Overlay settings now expose a focused retry action when output capture errors,
with explicit guidance that recovery waits for a fresh accepted snapshot.
Portable coverage is in
`python tests/unit/check_overlay_capture_recovery_action_contract.py`.

The overlay now reports the selected output endpoint and explicitly states that
combined-output monitoring does not identify individual decks. Portable coverage
is in `python tests/unit/check_overlay_endpoint_status_contract.py`.

The overlay now exposes dynamic keyboard recovery guidance for click-through
mode, including Escape and the control-window unlock shortcut, while keeping
interactive-mode guidance explicit. Portable coverage is in
`python tests/unit/check_overlay_keyboard_recovery_contract.py`.

Changing the Library source now clears stale visualization selection before the
overlay can reuse it, including recursive-browser selection. The overlay waits
for fresh accepted evidence instead of presenting the previous source's state.
Portable coverage is in
`python tests/unit/check_overlay_source_change_clear_contract.py`.

Overlay settings now provide a synchronized start/stop monitoring action. The
action preserves the control surface during transitions and announces that the
overlay is waiting for accepted combined-output data. Portable coverage is in
`python tests/unit/check_overlay_monitoring_action_contract.py`.

The overlay now exposes a recovery-action status that distinguishes a pending
display recovery from a safe restore action and preserves the control-window
fallback. Portable coverage is in
`python tests/unit/check_overlay_recovery_action_status_contract.py`.

The overlay now exposes whether it is available or hidden by user request,
with recovery guidance that preserves monitoring and restores persisted overlay
settings. Portable coverage is in
`python tests/unit/check_overlay_visibility_status_contract.py`.

The overlay now exposes whether an accepted visualization snapshot is ready,
distinguishing a live estimate surface from a waiting/replacement state.
Portable coverage is in
`python tests/unit/check_overlay_data_readiness_contract.py`.

The overlay settings surface now exposes truthful live-monitoring state,
including whether combined-output estimates are updating or capture is waiting
for accepted data. Portable coverage is in
`python tests/unit/check_overlay_live_monitoring_state_contract.py`.

Live capture errors now announce a concrete next action—retry monitoring after
checking the selected output and permission state—alongside the existing error
status and recovery controls.

Live Starting and Reacquiring transitions now announce their next action:
waiting for the first accepted snapshot or waiting for reacquisition after the
previous visualization selection was cleared.

Stopping a PipeWire monitor now marks the next restart boundary discontinuous
before closing callback admission. Portable coverage is in
`python tests/unit/check_pipewire_stop_discontinuity_contract.py`.

PipeWire control code now exposes a `needs_restart` predicate combining native
callback errors and closed admission, so lifecycle code can force a fresh epoch
instead of consuming stale transport state. Portable coverage is in
`python tests/unit/check_pipewire_restart_status_contract.py`.

When live capture transitions into Tracking, the control window now announces
that monitoring is active and visualization will update as accepted data
arrives, rather than leaving assistive technology on a silent transition.

The visualization now exposes its responsive compact/expanded layout state as
an accessible status, so width-driven presentation changes are announced
without implying a change to the underlying accepted analysis data.

The selected FFT bin now has a visible and screen-reader readout showing its
accepted magnitude. Wheel, keyboard, source-change, and result-reset paths all
update or clear the same readout state.

The PipeWire callback seam now records native error codes in an atomic status
slot, allowing control-thread lifecycle code to distinguish callback failure
from ordinary stop and restart. No allocation occurs on the callback path.
Portable coverage is in
`python tests/unit/check_pipewire_callback_error_status_contract.py`.

The FFT selection cursor now responds to mouse-wheel navigation with one-bin,
five-bin Shift, and ten-bin Control steps; waveform wheel selection retains
its time-position behavior.

Linux now has an explicit `MonitorBackend` boundary for PipeWire output
capture. The provisional adapter wires bounded sink discovery into that
control-thread seam and documents epoch, ownership, stop/quiescence, and
discontinuity contracts; stream transport remains the next implementation
slice. Portable coverage is in
`python tests/unit/check_pipewire_monitor_boundary_contract.py`.

PipeWire sink descriptions are now bounded to the same 4096-byte identity
limit, falling back to the validated node name when exceeded. Portable coverage
is in `python tests/unit/check_pipewire_description_length_contract.py`.

PipeWire sink serials and names are now bounded to 4096 bytes before entering
the registry snapshot, preventing unbounded metadata from reaching UI/source
selection. Portable coverage is in
`python tests/unit/check_pipewire_identity_length_contract.py`.

PipeWire descriptions containing NUL bytes now fall back to the validated node
name, preventing malformed display labels from reaching the UI. Portable
coverage is in `python tests/unit/check_pipewire_description_nul_contract.py`.

PipeWire discovery now rejects NUL-containing serial and node-name identities
before they reach source selection, matching the malformed-identity guards on
other capture backends. Portable coverage is in
`python tests/unit/check_pipewire_identity_nul_contract.py`.

PipeWire sink descriptions that are empty or whitespace-only now fall back to
the validated node name, preventing unusable display labels. Portable coverage
is in `python tests/unit/check_pipewire_description_validation_contract.py`.

PipeWire discovery now rejects whitespace-only node names as well as malformed
serials, ensuring every surfaced sink has a usable display identity. Portable
coverage is in
`python tests/unit/check_pipewire_sink_name_validation_contract.py`.

PipeWire registry discovery now rejects whitespace-only node serials before
adding sinks, preventing malformed identities from entering source selection.
Portable coverage is in
`python tests/unit/check_pipewire_serial_validation_contract.py`.

PipeWire sink ordering now uses the display name as a deterministic tie-breaker
when server serials collide, keeping refresh order stable even for duplicate
serial metadata. Portable coverage is in
`python tests/unit/check_pipewire_sink_tie_order_contract.py`.

PipeWire sink discovery now returns sinks in stable server-serial order rather
than volatile registry object-ID order, preventing UI selection churn across
refreshes. Portable coverage is in
`python tests/unit/check_pipewire_sink_order_contract.py`.

PipeWire discovery now rejects timeout durations that cannot be represented by
the loop's bounded integer wait, avoiding overflow in control-thread iteration.
Portable coverage is in
`python tests/unit/check_pipewire_timeout_upper_bound_contract.py`.

PipeWire discovery now rejects zero or negative control-thread timeouts before
initializing the runtime, returning a deterministic diagnostic instead of
running an undefined immediate-deadline loop. Portable coverage is in
`python tests/unit/check_pipewire_timeout_contract.py`.

PipeWire discovery now pairs each control-thread `pw_init` with scoped
`pw_deinit`, including early loop/context/registry failures, preventing global
PipeWire runtime leaks across repeated discovery. Portable coverage is in
`python tests/unit/check_pipewire_shutdown_contract.py`.

Core Audio stop requests made during PCM delivery now return typed `busy`
status with an actionable diagnostic instead of a silent false result.
Portable coverage is in
`python tests/unit/check_macos_stop_delivery_contract.py`.

Core Audio stop failures now expose an explicit retry-before-restart diagnostic
instead of retaining stale startup text. Portable coverage is in
`python tests/unit/check_macos_stop_failure_message_contract.py`.

Core Audio tap startup now reports distinct diagnostics for malformed output
UIDs versus a zero capture epoch, making invalid lifecycle requests actionable.
Portable coverage is in
`python tests/unit/check_macos_epoch_diagnostic_contract.py`.

The Core Audio tap now independently rejects empty, whitespace-only, and
NUL-containing UIDs before native setup, protecting direct backend callers as
well as the source adapter. Portable coverage is in
`python tests/unit/check_macos_tap_id_validation_contract.py`.

Invalid Core Audio tap arguments now map to the public `not_found` source
category instead of an opaque backend error, matching malformed source-ID
handling. Portable coverage is in
`python tests/unit/check_macos_invalid_argument_mapping_contract.py`.

Core Audio start requests now reject source IDs containing NUL bytes before
native NSString conversion, avoiding malformed identity lookups. Portable
coverage is in `python tests/unit/check_macos_source_id_nul_contract.py`.

Core Audio start requests now reject whitespace-only source IDs as
`not_found`, preventing malformed selections from reaching native tap setup.
Portable coverage is in
`python tests/unit/check_macos_source_id_whitespace_contract.py`.

Core Audio start requests now distinguish missing source IDs (`not_found`) and
missing consumers from an unavailable backend, giving callers actionable typed
diagnostics. Portable coverage is in
`python tests/unit/check_macos_invalid_source_contract.py`.

Core Audio startup now preserves the original typed failure (including
permission or device loss) even if backend cleanup also fails; the diagnostic
still appends the cleanup failure. Portable coverage is in
`python tests/unit/check_macos_start_error_preservation_contract.py`.

Core Audio interruption status now distinguishes a disappeared device/stream
from format or callback faults, making reacquisition guidance actionable.
Portable coverage is in
`python tests/unit/check_macos_status_message_contract.py`.

Cancelled Core Audio enumeration now normalizes both its typed error and its
diagnostic text, replacing stale backend text with a consistent cancellation
message. Portable coverage is in
`python tests/unit/check_macos_cancel_diagnostic_normalization_contract.py`.

Cancelled Core Audio enumeration now normalizes any stale backend error code to
the typed `cancelled` category, keeping outcome and error semantics aligned.
Portable coverage is in
`python tests/unit/check_macos_cancel_error_normalization_contract.py`.

Successful Core Audio enumeration now clears stale error categories for both
empty and non-empty success, keeping the result contract self-consistent.
Portable coverage is in
`python tests/unit/check_macos_success_error_clear_contract.py`.

Cancelled Core Audio enumeration now carries the typed `cancelled` source
error even when the backend omitted one, alongside its empty source list and
diagnostic. Portable coverage is in
`python tests/unit/check_macos_cancel_error_type_contract.py`.

Cancelled Core Audio enumeration now always clears any partial source list,
preventing stale devices from surviving a cancelled refresh. Portable coverage
is in `python tests/unit/check_macos_cancel_sources_empty_contract.py`.

Cancelled Core Audio enumeration now carries an explicit diagnostic when the
backend omits one, keeping cancellation distinct from failure and empty state.
Portable coverage is in
`python tests/unit/check_macos_enumeration_cancel_contract.py`.

Successful non-empty Core Audio enumeration now clears any stale diagnostic,
while empty success retains its explicit empty-state text. Portable coverage is
in `python tests/unit/check_macos_enumeration_success_clear_contract.py`.

Successful Core Audio enumeration with zero devices now carries an explicit
empty-state diagnostic, distinct from backend failure. Portable coverage is in
`python tests/unit/check_macos_empty_enumeration_diagnostic_contract.py`.

Core Audio enumeration errors now always carry a typed `backend_error` when a
backend omits its category, alongside the empty source list and fallback text.
Portable coverage is in
`python tests/unit/check_macos_enumeration_error_type_contract.py`.

Enumeration errors now clear any backend-provided source list before reaching
the UI, preventing stale devices from being offered alongside an error.
Portable coverage is in
`python tests/unit/check_macos_enumeration_error_empty_contract.py`.

Core Audio enumeration now guarantees a user-facing fallback diagnostic when a
backend reports an error without text, avoiding a silent unavailable-device
state. Portable coverage is in
`python tests/unit/check_macos_enumeration_diagnostic_contract.py`.

The FFT view now supports direct bin selection: clicking a spectrum bar or
using Left/Right while focused selects a bounded frequency bin, draws a cursor,
and exposes the selected bin through accessibility state. Waveform selection
continues to control playback position independently.

Waveform rendering now clamps accepted min/max PCM extrema to the normalized
[-1, 1] display range before mapping them into the canvas, preventing malformed
or transient out-of-range source values from escaping the visual bounds.

The FFT view now derives a bounded peak-bin summary from the accepted spectrum
snapshot and exposes it through the canvas accessibility value, making the
rendered frequency evidence inspectable without inventing a calibrated pitch.

FFT rendering now clamps negative or malformed magnitudes to zero before
painting bars, preventing invalid source values from producing inverted
frequency visuals while preserving the bounded real-spectrum path.

`Ctrl+Alt+Shift+W` focuses Restore overlay, preserving a direct keyboard path
to bring back a user-hidden overlay with its persisted settings.

When the native overlay is unavailable, `Ctrl+Alt+Shift+Y` focuses the Retry
overlay action so recovery remains keyboard reachable.

`Ctrl+Alt+Shift+P` focuses the Reapply saved settings action, providing a
keyboard-first path to restore persisted display, geometry, size, and opacity.

Overlay shortcut guidance now covers every keyboard recovery and visual-control
path: unlock, click-through, show/hide, display selection, retry, size,
opacity, and saved-settings reapply.

When click-through is active, `Ctrl+Alt+U` focuses the Unlock overlay action
and announces that Enter will restore keyboard and pointer interaction.

`Ctrl+Alt+J` focuses the Hide overlay action, keeping the control-window
fallback keyboard reachable during an active overlay session.

`Ctrl+Alt+W` focuses the Show overlay action, providing a direct keyboard path
to restore the native overlay after recovery or configuration changes.

`Ctrl+Alt+Z` focuses the overlay size selector and announces the active
Compact, Medium, or Large preset before keyboard adjustment.

`Ctrl+Alt+K` focuses the overlay click-through control and announces whether
pointer passthrough is currently enabled or disabled.

`Ctrl+Alt+L` focuses the overlay opacity slider and announces its current
percentage, providing a direct keyboard path to adjust visual prominence.

Shortcut guidance now also lists keyboard size cycling (`Ctrl+Alt+O`) and the
90% opacity reset (`Ctrl+Alt+Shift+O`), keeping visual overlay controls
discoverable without requiring pointer access.

Overlay shortcut guidance now includes click-through toggling, display focus,
and retry recovery alongside unlock, hide/restore, and saved-settings reapply,
so every keyboard recovery path is visible and screen-reader discoverable.

`Ctrl+Alt+I` toggles native overlay click-through from the keyboard. Successful
changes announce whether interaction is disabled or restored; rejected changes
retain the current mode and report recovery guidance.

When the native overlay is unavailable but displays are enumerated, `Ctrl+Alt+Y`
retries recovery while keeping the control window visible until the show call
succeeds, matching the button-based fallback path.

`Ctrl+Alt+D` focuses the overlay display selector and announces whether a
replacement display is required, providing a direct keyboard recovery path
when a monitor disappears or a saved destination becomes stale.

The control window now visibly lists the overlay keyboard shortcuts for
unlocking click-through, hiding/restoring, and reapplying saved settings, with
the same text exposed as an accessible status value.

Saved overlay display, size, position, and opacity can be reapplied from the
keyboard with `Ctrl+Alt+Shift+R`, using the same recovery path as the control
window and announcing the restoration state.

The overlay can be hidden or restored from the keyboard with `Ctrl+Alt+H`.
The shortcut uses the same persisted hide/restore path as the control-window
buttons, keeping monitoring state and the discoverable fallback synchronized.

The overlay opacity can be reset from the keyboard with `Ctrl+Alt+Shift+O`.
The shortcut uses the native persistence bridge and announces either the 90%
reset or a recovery message when the native edit is rejected.

The overlay size presets are keyboard reachable with `Ctrl+Alt+O`, cycling
Compact, Medium, and Large through the native resize bridge. Rejected resizes
keep persisted geometry authoritative and announce recovery.

Overlay size changes now treat native resize failure as a rejected edit: the
persisted geometry remains authoritative, the size selector is synchronized
back to that geometry, and the control window announces the recovery instead
of reporting a size that was not applied.

macOS Core Audio discovery now places the system-selected output first after a
successful enumeration, without changing the neutral `SourceInfo` contract or
overriding a later explicit user selection. The portable source contract is
checked by `python tests/unit/check_macos_default_output_order_contract.py`;
native Core Audio permission and tap qualification remain required.

## Current recovery/cache qualification snapshot

The current storage recovery path persists retryable state for cancellation,
stale verification, empty publication, oversized publication, and transactional
publish failures. Portable evidence for this slice is:

```text
python tests/unit/check_publish_cancellation_recovery_contract.py
python tests/unit/check_publish_oversize_recovery_contract.py
python tests/unit/check_publish_empty_recovery_contract.py
python tests/unit/check_sqlite_publish_contract.py
```

These checks verify source contracts only; they do not replace Catch2 execution
or native Windows, macOS, and Linux qualification. Native evidence remains
required for the issue #1 exit gate.

Shortcut-triggered stop/resume must announce the resulting lifecycle state through the same accessible status surface as pointer actions. Portable contracts cover focus, shortcut dispatch, and state text; native screen-reader timing, focus restoration, and assistive-technology behavior remain unqualified.

Keyboard playback recovery parity is part of the readiness handoff: transport, retry, endpoint refresh, and clear/reacquire actions must be reachable through the same focus order and accessible names as pointer controls. Portable UI contracts document this parity; native assistive-technology and hardware recovery remain unqualified.

Playback recovery controls and the storage bridge transition surface are now documented as one handoff: queued/running/completed/cancelled/failed states remain distinct, stale results clear across epoch or endpoint transitions, and retry controls require preserved identity. Local transition contracts are preparation evidence; native playback, endpoint-loss, and overlay behavior remain open.

Retry decisions are now explicit: a persisted run is resubmitted only when source path, content identity, engine, configuration, mode, and decoder snapshot are present and compatible; cache hits complete without decoder work; missing or mismatched metadata rejects fail-closed. Local checks are `python tests/unit/check_bridge_resubmit_contract.py`, `python tests/unit/check_retry_evidence_docs.py`, and `git diff --check`. These portable checks do not qualify native codecs, devices, or desktop recovery.

Decoder retry evidence now has a paired handoff expectation: compatible snapshot/identity retries may proceed, while changed or legacy decoder snapshots must reject. Both outcomes need retained commands and diagnostics before native qualification can advance.

Native codec/runtime preparation is now explicit: qualification artifacts must bind decoder snapshot, codec feature configuration, fixture hash, backend/device, build identity, and exact commands. Both successful decode and controlled failure evidence are required; portable tests remain preparation evidence and native gates stay open.

Decoder snapshot mismatch handling is explicit: retry provenance requires exact compatibility with the active decoder identity. Empty legacy snapshots and mismatches reject recovery rather than silently decoding with a different runtime; the portable contract does not qualify native codec behavior.

Decoder compatibility is explicit in retry recovery: `decoder_snapshot` travels with the persisted run, and legacy retry rows with an empty snapshot remain rejected rather than silently reusing a changed decoder. Local checks cover this policy; native codec/runtime compatibility is still unqualified.

Retryable storage schema v2 persists `source_path` and `decoder_snapshot` with each run and migrates existing databases by adding empty defaults. Recovery UI and coordinator resubmission consume these fields conservatively; empty legacy metadata remains non-retryable until a source operation is reconstructed. This migration is covered by portable storage contracts, not native qualification.

Playback/recovery behavior now treats an epoch change or endpoint interruption as a lifecycle boundary: stale queued evidence is cleared, reacquisition starts fresh, and prior results are not presented as current. Portable lifecycle contracts cover this behavior; native device-loss and real playback observations remain unqualified.

## Qualification handoff pipeline

The merged portable handoff pipeline is now explicit: native platform scripts produce retained artifacts; `tests/platform/ingest_qualification_artifacts.py` verifies their hashes and identity into conservative manifests; `tests/platform/run_readiness_handoff.py` generates and validates the readiness report; and `tests/platform/consume_readiness_handoff.py` is the downstream acceptance boundary. Ingestion, documentation, and readiness failures remain auditable and nonzero, and generated scenarios remain `PENDING` until real native desktop/audio/overlay observations are reviewed. This pipeline is preparation evidence, not native qualification.

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

The accepted handoff leaves actual capture source adapters, clock anchoring and interruption semantics, tested store and presentation implementations, reproducible pinned platform dependencies and native qualification as open work.

## Tracking lifecycle and gate audit increment

`TrackingMachine` is worker-owned and guards results with epoch/generation tokens. Replay now clears the tempo and resets the tracker after one second of peak-below-0.0001 signal, and reacquires after audio resumes. This threshold is a provisional tuning choice; it is not a real-music accuracy guarantee. Generated replay tests cover acquisition, silence and resume. [The gate audit](phase0-gate.md) lists remaining concrete work and proposed ownership; the gate stays closed.

- Windows x64, MSVC 19.44, existing vcpkg development libraries: `cmake --build build-slice --config Debug -j 6` and `ctest --test-dir build-slice -C Debug --output-on-failure`: 18/18 passed.
- Ubuntu 22.04 WSL2 with Qt enabled: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 19/19 passed. This is not native Linux qualification.
- Windows pinned desktop: `VCPKG_ROOT=C:/Users/zachf/scoop/apps/vcpkg/current` and `cmake --preset vcpkg-desktop -G "Visual Studio 17 2022" -A x64` failed in ICU 78 during the debug install step: `make` reported `./../src/icu4c-78-34dc699e15.clean/source/install-sh: No such file or directory`, although the source tree contains that file. The log is `build/vcpkg-desktop/vcpkg-manifest-install.log`; configuration/build/test and Qt deployment remain unverified. The build used the shared Scoop vcpkg tree. Intel macOS Darwin 25.6.0 over SSH, build a0befdb with the isolated aubio and Homebrew Qt/libsndfile setup described above: the same CMake configure command, `cmake --build /tmp/beatglass-phase0-lifecycle-build -j 4` and `ctest --test-dir /tmp/beatglass-phase0-lifecycle-build --output-on-failure` passed 19/19; linker warned `/lib` was absent. No interactive GUI or actual audio was tested.

## Immutable snapshot publication increment

`SnapshotPublisher` provides a worker-published latest immutable value, item/text limits, acquisition-token checks and validation of event order/epochs. It is not yet connected to live analysis or the GUI, and caller-owned old snapshots survive invalidation. It is not real-time safe. Windows x64 MSVC: `cmake --build build-slice --config Debug -j 6` and `ctest --test-dir build-slice -C Debug --output-on-failure`: 21/21 passed. Ubuntu 22.04 WSL2, Qt enabled: `cmake --build /tmp/beatglass-slice -j 6` and `ctest --test-dir /tmp/beatglass-slice --output-on-failure`: 22/22 passed. Intel macOS Darwin 25.6.0 via SSH, build 3dd513c using the isolated aubio and Homebrew Qt/libsndfile setup above: `cmake --build /tmp/beatglass-phase0-lifecycle-build -j 4` and `ctest --test-dir /tmp/beatglass-phase0-lifecycle-build --output-on-failure`: 22/22 passed; linker warned of a duplicate core library and absent `/lib` search path. This is not native GUI/audio qualification.

The strict documentation workflow for the snapshot increment ran `python docs/api/check_headers.py`, `docker build -f docs/api/Dockerfile -t beatglass-docs .`, `docker run --rm --mount type=bind,source=<repository>,target=/work beatglass-docs sphinx-build -E -W --keep-going -b html docs/api /work/build/docs/html`, and `python docs/api/check_rendered.py build/docs/html/index.html`: all passed, covering 17 headers and 50 rendered names.

## Windows Qt desktop spike

With code at 1de29dd, Windows x64/MSVC 19.44 and Qt 6.8.3 prebuilt SDK installed to a temporary directory using `python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir <temp>/beatglass-qt --archives qtbase qtdeclarative qtshadertools qttools`, the following passed: `cmake -S . -B build-qt-spike -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="<temp>/beatglass-qt/6.8.3/msvc2022_64;C:/Users/zachf/scoop/apps/vcpkg/current/installed/x64-windows"`, `cmake --build build-qt-spike --config Debug -j 6`, `ctest --test-dir build-qt-spike -C Debug --output-on-failure`: 22/22 passed. windeployqt copied the Windows platform plugin and QML modules; it warned that the SDK's translations catalog was absent. With PATH restricted to Windows system directories and `QT_QPA_PLATFORM=windows`, the deployed desktop process stayed running for three seconds without startup errors. An attempted offscreen startup failed because windeployqt deployed only the Windows platform plugin. No interactive file dialog, audio or overlay was tested. This prebuilt Qt 6.8.3 spike does not verify the pinned Qt 6.11.1 manifest, whose ICU build failure is recorded above.

## Experimental v1 codec features

The proposed vcpkg manifest enables libsndfile's `external-libs` and `mpeg` features. Three generated, redistributable AIFF/FLAC/MP3 fixtures exercise actual decoded replay frames, metadata and source hashes; these do not qualify all format variants or musical accuracy. On the Intel Mac over SSH, with the existing Homebrew libsndfile 1.2.2 and CLI build aa9ef95, `python3 /tmp/beatglass-codec-formats.py /tmp/beatglass-file-reader-build/beatglass-cli /tmp/beatglass-codec-fixtures` passed for all three fixtures after copying the generated files and test script to those temporary paths. This was not a pinned vcpkg or native audio test.

The previous Windows core-only libsndfile build opened the AIFF fixture but failed to open the FLAC and MP3 fixtures; that confirms the prior manifest configuration did not meet v1 format coverage. With merged code b744cd0 on Ubuntu 22.04 WSL2, GCC 11.4 and pinned vcpkg libsndfile 1.2.2 with both codec features, `cmake -S /mnt/c/Users/zachf/.codex/worktrees/beatglass-codecs -B /tmp/beatglass-pinned-codecs -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=/tmp/beatglass-vcpkg-codecs/scripts/buildsystems/vcpkg.cmake -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, `cmake --build /tmp/beatglass-pinned-codecs -j 3`, and `ctest --test-dir /tmp/beatglass-pinned-codecs --output-on-failure` passed 27/27, including all three codec fixtures. WSL is not native Linux qualification. Windows pinned codec tests and release-license review remain pending. Ubuntu 22.04's system libsndfile 1.0.31 predates upstream MP3 support; skipping codec integration tests for that development setup does not qualify it.
## Pull-based file reader increment

`FileReader` decodes into caller-owned float storage without a hardware callback. Successful seeks advance the caller-seeded epoch; EOF and cancellation before I/O are explicit, and failed reads/seeks invalidate the reader. Cancellation cannot interrupt an already-blocking decoder read. Existing file analysis/replay uses this reader. Generated WAV tests exercise sequential reads, EOF, seek/reacquire, invalid inputs, cancellation, epoch overflow, separate 44.1/48 kHz epochs and byte-for-byte source preservation.

- Windows x64/MSVC, vcpkg development libraries: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Users/zachf/scoop/apps/vcpkg/current/installed/x64-windows -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build --config Debug -j 4`, `ctest --test-dir build -C Debug --output-on-failure`: 26/26 passed. The Catch2 source override reuses the separately downloaded, pinned v3.8.1 source.
- Ubuntu 22.04 WSL2 with distribution libraries: `cmake -S /mnt/c/Users/zachf/.codex/worktrees/beatglass-file-replay -B /tmp/beatglass-file-replay-build -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, `cmake --build /tmp/beatglass-file-replay-build -j 3`, `ctest --test-dir /tmp/beatglass-file-replay-build --output-on-failure`: 26/26 passed. WSL is not native Linux qualification.
- Strict docs: `python docs/api/check_headers.py` covers 18 headers; the pinned Docker Sphinx build with `-E -W --keep-going` passed and `python docs/api/check_rendered.py build/docs/html/index.html` checked 65 declarations. Intel macOS Darwin 25.6.0 via SSH, build 7e965e9 with the isolated aubio and Homebrew Qt/libsndfile setup recorded above: `cmake --build /tmp/beatglass-file-reader-build -j 4` and `ctest --test-dir /tmp/beatglass-file-reader-build --output-on-failure` passed 27/27; linker warned that `/lib` was absent. Actual device capture, interactive playback and all native audio/overlay qualification remain unrun.

## Presentation ordering proposal

`PresentationGate` filters immutable snapshot updates and invalidations by strictly increasing producer-assigned revisions on the presentation thread. A delayed old snapshot cannot overwrite a delivered clear or newer snapshot; an older delayed clear cannot erase a newer snapshot. The producer must order all updates, handle revision exhaustion and deliver them through a suitable cross-thread queue. A newly created gate needs a queue drain or an external session guard. A clear takes effect only once delivered, and retained prior shared pointers remain valid but are no longer current. This helper is not connected to the Qt shell or a live source.

Ubuntu 22.04 WSL2 with GCC 11.4 and system decoder libraries: `cmake -S /mnt/c/Users/zachf/.codex/worktrees/beatglass-presentation-order -B /tmp/beatglass-presentation-build -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, `cmake --build /tmp/beatglass-presentation-build -j 3`, and `ctest --test-dir /tmp/beatglass-presentation-build --output-on-failure` passed 27/27. WSL is not native Linux qualification. Strict Hawkmoth Sphinx build passed with 19 headers and 67 checked declarations.

Windows x64/MSVC 19.44 with existing vcpkg development libraries and reused pinned Catch2 source: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Users/zachf/scoop/apps/vcpkg/current/installed/x64-windows -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build --config Debug -j 4`, and `ctest --test-dir build -C Debug --output-on-failure` passed 27/27. No Qt UI integration was exercised.
## Job lifecycle proposal

`JobLifecycle` is a single-worker state machine, not a scheduler or database. It represents an unknown total separately from zero, rejects regressing or over-total frame counts, and makes cancellation, failure and completion terminal. The worker serializes terminal decisions. For persistent analysis, the caller must only mark completion after a successful transaction; this helper cannot enforce file-version checks, atomic commit, or cancellation arbitration inside a database. Those remain open implementation and integration requirements.

Ubuntu 22.04 WSL2 with GCC 11.4 and system decoder libraries: `cmake -S /mnt/c/Users/zachf/.codex/worktrees/beatglass-job-lifecycle -B /tmp/beatglass-job-lifecycle-build -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, `cmake --build /tmp/beatglass-job-lifecycle-build -j 3`, and `ctest --test-dir /tmp/beatglass-job-lifecycle-build --output-on-failure` passed 28/28. WSL is not native Linux qualification. Strict Hawkmoth Sphinx build passed with 19 headers and 67 checked declarations.

Windows x64/MSVC 19.44 with existing vcpkg development libraries and reused pinned Catch2 source: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Users/zachf/scoop/apps/vcpkg/current/installed/x64-windows -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build --config Debug -j 4`, and `ctest --test-dir build -C Debug --output-on-failure` passed 28/28.
## Source lifecycle proposal

`SourceLifecycle` is a control-thread-only state helper, not a live capture adapter. Each start returns a new attempt token; late notifications from earlier attempts are rejected. A successful start or resume must supply an epoch strictly greater than any previously accepted by that object. Interruption and failure invalidate its current epoch. Stop invalidates its state, but the adapter still must quiesce its callbacks before releasing callback-owned data. Notifications for the current attempt must be ordered by the adapter; the token does not distinguish multiple notifications within one attempt. Object destruction/recreation requires an external epoch allocator if epoch uniqueness must persist.

A unit scenario explicitly stops tracking and clears the snapshot publisher on interruption, then restarts tracking on a new epoch; stale results cannot repopulate the cleared publisher. This tests the intended handoff in isolation, not an integrated hardware interruption path. WSL Ubuntu 22.04, GCC 11.4 and system decoder libraries: `cmake -S /mnt/c/Users/zachf/.codex/worktrees/beatglass-source-lifecycle -B /tmp/beatglass-source-lifecycle-build -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, `cmake --build /tmp/beatglass-source-lifecycle-build -j 3`, `ctest --test-dir /tmp/beatglass-source-lifecycle-build --output-on-failure`: 29/29 passed. This is not native Linux qualification. Strict Hawkmoth Sphinx build passed with 19 headers and 67 checked declarations.

Windows x64/MSVC 19.44 with existing vcpkg development libraries and the pinned Catch2 source reused from the main worktree: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Users/zachf/scoop/apps/vcpkg/current/installed/x64-windows -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build --config Debug -j 4`, `ctest --test-dir build -C Debug --output-on-failure`: 29/29 passed. No live device was exercised.

After merging current main at 452bb6f, `cmake --build build --config Debug -j 4` and `ctest --test-dir build -C Debug --output-on-failure` passed 30/30 on Windows. `cmake --build /tmp/beatglass-presentation-build -j 3` and `ctest --test-dir /tmp/beatglass-presentation-build --output-on-failure` passed 30/30 on WSL. The strict Docker Sphinx build and rendered declaration check passed with 20 indexed headers and 69 checked declarations. Both source and presentation tests remain in CMake and both headers remain in the API index.
After merging current main at 92c0f02, `cmake --build build --config Debug -j 4` and `ctest --test-dir build -C Debug --output-on-failure` passed 31/31 on Windows. `cmake --build /tmp/beatglass-job-lifecycle-build -j 3` and `ctest --test-dir /tmp/beatglass-job-lifecycle-build --output-on-failure` passed 31/31 on WSL. The strict Docker Sphinx build and rendered declaration check passed with 20 indexed headers and 69 checked declarations. Both source and job tests remain in CMake and both headers remain in the API index.

## Prebuilt Qt 6.8.3 candidate

On Windows x64, MSVC 19.44 and a clean, isolated vcpkg checkout at baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`, Qt 6.8.3 `win64_msvc2022_64` from the Qt repository was configured with `BEATGLASS_QT_ROOT=<temp>/beatglass-qt/6.8.3/msvc2022_64` and `VCPKG_ROOT=C:/Users/zachf/.codex/worktrees/beatglass-vcpkg-prebuilt`. `cmake --preset prebuilt-qt-desktop -G "Visual Studio 17 2022" -A x64 -B build/prebuilt-qt-clean -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build/prebuilt-qt-clean --config Debug -j 4`, and `ctest --test-dir build/prebuilt-qt-clean -C Debug --output-on-failure` passed 32/32; `windeployqt` ran. The SDK's `Qt6Core.dll` SHA-256 was `0cc62e841959fb8f6d366ea116c6f6bbc273e30f0ef90d8aa2e4adeb81fd63f8` and `windeployqt.exe` was `46dad230273dbf57f4d56c719eff26f3bdf40cc1628972f057b485cd4b9380c3`; these identify the observed files, not a complete archive-integrity lock.

A first configure reused an install tree containing partial Qt 6.11.1 and failed from mixed `Qt6CoreTools`; the clean tree passed. A Qt 6.8.3 Linux SDK was downloaded on WSL, but its build/test was interrupted at the user's request to release vcpkg locks; CMake had reported missing XKB development files before interruption. No Linux prebuilt build success or native Linux qualification is claimed. macOS prebuilt SDK build/testing remains unrun. See the dependency decision for acquisition, packaging and license limitations.

A separate Windows configure with `BEATGLASS_QT_VERSION=6.8.4`, `Qt6_DIR` pointing at the 6.8.3 SDK, `BUILD_TESTING=OFF`, and `VCPKG_MANIFEST_INSTALL=OFF` failed as expected because the exact requested Qt version did not match. This negative check used the existing core-only install tree and did not start a vcpkg install.

The parallel Qt 6.11.1 vcpkg source attempt (PR #11) built ICU, Qt base, SVG, shader tools and language server but failed in Qt Declarative debug with MSVC C1083 on missing `qtquickcontrols2fluentwinui3styleimplplugin_QtQuickControls2FluentWinUI3StyleImplPlugin.moc`. The build warned about long paths; the cause is not established. The complete source-built desktop did not pass. PR #11 is superseded as the preferred development route if this prebuilt proposal is accepted; its historical workaround and platform research may still be reviewed separately.
After merging PR #15 at ef4f3ff, `cmake --build build --config Debug -j 4` and `ctest --test-dir build -C Debug --output-on-failure` passed 32/32 on Windows. `cmake --build /tmp/beatglass-presentation-build -j 3` and `ctest --test-dir /tmp/beatglass-presentation-build --output-on-failure` passed 32/32 on WSL. The strict Docker Sphinx build and rendered declaration check passed with 21 indexed headers and 71 checked declarations. The source, job and presentation test files and API index entries are all retained.

After merging current main at 017dad3, `cmake -S . -B build/prebuilt-qt-clean -DVCPKG_MANIFEST_INSTALL=OFF`, `cmake --build build/prebuilt-qt-clean --config Debug -j 4`, and `ctest --test-dir build/prebuilt-qt-clean -C Debug --output-on-failure` passed 33/33 on Windows with the same already-installed pinned core packages and Qt 6.8.3 SDK. Disabling manifest installation for this rerun avoided acquiring a vcpkg lock; the original clean configure above had installed the manifest normally.
After merging source and job work at 3adb001, `cmake --build /tmp/beatglass-pinned-codecs -j 3` and `ctest --test-dir /tmp/beatglass-pinned-codecs --output-on-failure` passed 32/32 on Ubuntu WSL with the pinned codec features, including the AIFF, FLAC and MP3 integration test. Strict Docker Sphinx documentation and rendered checks passed for 20 indexed headers and 69 checked declarations. WSL is not native Linux qualification.

Windows pinned codec verification on 2026-11-07: with MSVC 19.44, an isolated shared clone of the same vcpkg checkout/baseline at `C:/Users/zachf/.codex/worktrees/beatglass-vcpkg-codecs-win`, `VCPKG_MAX_CONCURRENCY=3`, and merged source at fda5bff, `cmake -S . -B build-pinned-win -G "Visual Studio 17 2022" -A x64 -DBEATGLASS_BUILD_SLICE=ON -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=C:/Users/zachf/.codex/worktrees/beatglass-vcpkg-codecs-win/scripts/buildsystems/vcpkg.cmake -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build-pinned-win --config Debug -j 3`, and `ctest --test-dir build-pinned-win -C Debug --output-on-failure` passed 32/32, including `cli_v1_codecs` for synthetic AIFF, FLAC and MP3 with source hashes unchanged. This does not establish broader format or native audio qualification.
After merging PR #15 at ef4f3ff, `cmake --build build --config Debug -j 4` and `ctest --test-dir build -C Debug --output-on-failure` passed 32/32 on Windows. `cmake --build /tmp/beatglass-presentation-build -j 3` and `ctest --test-dir /tmp/beatglass-presentation-build --output-on-failure` passed 32/32 on WSL. The strict Docker Sphinx build and rendered declaration check passed with 21 indexed headers and 71 checked declarations. The source, job and presentation test files and API index entries are all retained.

## Interactive macOS metadata shell smoke

On 2026-11-07, the Intel Mac (`macOS 26.7.1`, build `25G241`, x86_64) was accessed through its logged-in graphical desktop via VNC. The existing `/tmp/beatglass-file-reader-build/beatglass-desktop` was built from the file-reader source snapshot previously recorded at 7e965e9, with Homebrew Qt 6.12.0, libsndfile and the isolated aubio setup. The source was copied to `/tmp/beatglass-file-reader-src` without `.git`, so the executable hash is the directly verified build identity: SHA-256 `8111f117be1bc7bff13e1c16c5818e05b7a48e7695c4427619ce16525b58bbd3`. A temporary application bundle containing an identical executable was launched using `open -n /tmp/BeatglassSmoke.app` to bring it into the graphical session; this is a smoke harness, not a distributable or packaged app.

Through the native file picker, a generated mono 44.1 kHz, 1-second WAV in `/tmp` was selected. The shell displayed `WAV | 44100 Hz | 1 channels | 1.00 seconds`. Its SHA-256 before and after was `3fa20276d8a4131431490ed129d0b05fafa4e9c82d4339b5cf635512103d6615`. This verifies the minimal interactive metadata shell only; it is not a pinned Qt SDK build, installer/package check, audio playback, system-output capture, overlay, library/DJ workflow or v1 qualification.
After merging PR #16 at fdba318, the Windows pinned codec build regenerated and compiled the combined test executable before its CTest run was interrupted to release vcpkg locks. A subsequent lock-free `ctest --test-dir build-pinned-win -C Debug --output-on-failure` passed 33/33, including the source, job, presentation and AIFF/FLAC/MP3 fixture checks. WSL's combined rebuild was interrupted and is not claimed for this merge.

On Windows 10.0.26300 x64, the deployed Qt 6.8.3/MSVC Debug desktop build from commit 1de29dd opened a generated one-second, mono 44.1 kHz WAV in the native file dialog and displayed `WAV | 44100 Hz | 1 channels | 1.00 seconds`. The synthetic fixture SHA-256 was `c375b6f1a80778e2c695dfb3494f742b40a0ffb2451e9cb7e2a4c383188b3cfc` before and after. This checks metadata selection only; it does not qualify playback, DJ capture, overlays, or a release package.

After merging the codec manifest in PR #13 at 2b74e90, a lock-free combined Windows test used the previously installed codec-enabled pinned vcpkg tree: with `VCPKG_ROOT=C:/Users/zachf/.codex/worktrees/beatglass-vcpkg-prebuilt` and `BEATGLASS_QT_ROOT=<temp>/beatglass-qt/6.8.3/msvc2022_64`, `cmake --preset prebuilt-qt-desktop -G "Visual Studio 17 2022" -A x64 -B build/prebuilt-qt-codecs -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_INSTALLED_DIR=C:/Users/zachf/.codex/worktrees/beatglass-codecs/build-pinned-win/vcpkg_installed -DFETCHCONTENT_SOURCE_DIR_CATCH2=C:/Users/zachf/.codex/worktrees/c429/beatglass/build-slice/_deps/catch2-src`, `cmake --build build/prebuilt-qt-codecs --config Debug -j 4`, and `ctest --test-dir build/prebuilt-qt-codecs -C Debug --output-on-failure` passed 34/34, including the desktop metadata worker and AIFF/FLAC/MP3 replay fixtures. Manifest installation was disabled only for this rerun to avoid a vcpkg lock; the codec tree had been installed from the pinned manifest as documented above.

After merging PR #18 at 2f66515 (documentation changes), the existing prebuilt Qt and codec-enabled Windows executable passed `ctest --test-dir build/prebuilt-qt-codecs -C Debug --output-on-failure` 34/34 without acquiring a vcpkg lock. The strict Docker Sphinx build passed with 21 indexed headers and 71 rendered declarations. The Mac smoke and current codec/Qt gate evidence are retained.

## Provisional SQLite and bounded worker review increment

On 2026-11-07, WSL Ubuntu 22.04 with GCC 11.4 and system SQLite 3.37.2, based on `6f927cb0eceb084a8333f6a90f25ccab84ada593`: `cmake -S . -B /tmp/beatglass-library-phase1 -DBUILD_TESTING=ON -DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, `cmake --build /tmp/beatglass-library-phase1 -j 3`, and `ctest --test-dir /tmp/beatglass-library-phase1 --output-on-failure` passed 23/23 CTest entries. The new entry exercises private prototype primitives, not a user workflow or real DSP. WSL is not native Linux qualification. SQLite and filesystem identity remain unpinned/unimplemented respectively; Windows and macOS checks were not run for this increment. See the Phase 1 proposal for schema limitations and remaining work.
The documentation check `docker run --rm -v "${PWD}:/work" beatglass-docs sphinx-build -W --keep-going -b html docs/api /tmp/beatglass-docs-html` passed using the existing docs image. No C++ headers were added or changed.

Review update: the same WSL configure/build/CTest commands now pass 30/30 individually discovered tests linked to `beatglass_library_prototype`; tests cover SQLite binding length bounds and a second connection rejected while the first holds the exclusive lock. The CI proposal enables the target on Ubuntu. Windows and macOS remain untested for this increment.

## Phase 1 shared UI navigation increment

On Windows 10.0.26300 x64, MSVC 19.44 and Qt 6.8.3, the shared desktop shell gained Library and DJ navigation while retaining real asynchronous file metadata inspection. Library indexing, analysis, playback, capture and overlay remain unavailable. From this worktree at base 6f927cb with `VCPKG_ROOT=C:/Users/zachf/.codex/worktrees/beatglass-vcpkg-prebuilt` and `BEATGLASS_QT_ROOT=$env:TEMP/beatglass-qt/6.8.3/msvc2022_64`, `cmake --preset prebuilt-qt-desktop -G "Visual Studio 17 2022" -A x64 -B "$env:TEMP/beatglass-ui-build" -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_INSTALLED_DIR=C:/Users/zachf/.codex/worktrees/beatglass-codecs/build-pinned-win/vcpkg_installed`, `cmake --build "$env:TEMP/beatglass-ui-build" --config Debug -j 4`, and `ctest --test-dir "$env:TEMP/beatglass-ui-build" -C Debug --output-on-failure` passed 35/35, including the offscreen Qt Quick navigation test. The existing installed vcpkg tree was reused; this is not a fresh dependency qualification. `docker build -f docs/api/Dockerfile -t beatglass-docs .` and `docker run --rm -v "${PWD}:/work" beatglass-docs sphinx-build -W --keep-going -b html docs/api /tmp/beatglass-docs-html` passed. No native interactive UI, audio, or overlay qualification was performed for this increment.

## Phase 1 filesystem browser and live presentation boundary

On Windows 10.0.26300 x64 with MSVC 19.44 and Qt 6.8.3, `cmake --build "$env:TEMP/beatglass-ui-build" --config Debug -j 4` and `ctest --test-dir "$env:TEMP/beatglass-ui-build" -C Debug --output-on-failure` passed 36/36 using the existing vcpkg installation described above. After extending the QML test to activate a file with the keyboard, `ctest --test-dir "$env:TEMP/beatglass-ui-build" -C Debug -R desktop_navigation --output-on-failure` passed. The offscreen test browses checked-in audio fixtures; it does not qualify the native folder dialog or desktop accessibility. The current directory view is not an indexed or persistent collection, and cannot report a precise reason for an unreadable or empty folder. The GUI-thread presentation adapter rejects older revisions and clears estimates on invalidation/non-tracking states; no live producer is connected. Database integration, real capture, overlay and native platform qualification remain pending.
The API index coverage check (`python docs/api/check_headers.py`) covered 22 headers; the warnings-as-errors Docker Sphinx build and `python docs/api/check_rendered.py "$env:TEMP/beatglass-docs-output/html/index.html"` passed. The new adapter is application-private and has no cross-thread delivery mechanism; integration must supply ordered GUI-thread delivery including clears and session/restart protection.

# Source contract integration

The proposed shared source types and ordered lifecycle helper have regression
coverage. Native adapters, control queues and actual source clock measurements
remain unimplemented and require integration and platform qualification.

PR #63 merged concurrent process-epoch issuance with contention and exhaustion
tests. The application must still share one allocator across live and offline
paths and coordinate each source's lifecycle.

# Cache contract integration

Typed cache outcomes and a worker-local commit decision have regression
coverage. SQLite, file-version revalidation, crash recovery and unchanged-file
reuse acceptance tests remain backend work.

# Shared timeline contract

The snapshot publisher enforces exclusive observed-frame and nonempty
half-open chord boundaries. The remaining Phase 1 contracts and native
qualification are still in progress.

# Phase 0 handoff decision

The coordinator accepted the Phase 0 handoff for starting Phase 1 on merged
commit `6f927cb`. Shared contracts, dependency and SDK integrity across
platforms, licensing, and native v1 qualification remain open.

## Phase 1 private tonal tracker

The optional private Essentia tracker is a worker-only experiment tested on generated triads and discontinuities. It is disabled by default and not connected to file analysis or live capture. Release licensing, Windows MSVC dependency build, key/chord quality, timestamped segment policy and native qualification remain open; see [tonal experiment](tonal-experiment.md).

## Shared causal tempo replay proposal

`CausalTempoReplay` consumes borrowed interleaved blocks on one worker, buffers at most one 512-frame hop, and runs the existing aubio tempo and signal lifecycle logic. File replay now uses this same engine; a future snapshot job can feed it without reopening the path. State reports both the exclusive accepted boundary and the last complete analyzed hop. At EOF an incomplete hop is not analyzed. Gaps and new epochs discard buffered evidence; old epochs, overlap, malformed PCM and same-epoch format changes fail closed. The caller must allocate epochs and deliver invalidation/clear notifications around source lifecycle events; the engine is not an audio callback, decoder, cache or GUI publisher. This is a proposed shared API pending integration-owner review. Tonal and timestamped beat trackers are separate unmerged increments and are not yet integrated into this engine.

## Phase 1 private tonal tracker

The optional private Essentia tracker is a worker-only experiment tested on generated triads and discontinuities. It is disabled by default and not connected to file analysis or live capture. Release licensing, Windows MSVC dependency build, key/chord quality, timestamped segment policy and native qualification remain open; see [tonal experiment](tonal-experiment.md).



## Experimental beat events

The worker-only aubio beat-event experiment reports epoch-local sample positions and the exclusive observed boundary for each detection. Its 1024-sample window/512-sample hop and aubio's estimated event time are not a measured latency guarantee. An event at the current exclusive boundary waits for a subsequent observed sample; it is not flushed at EOF. Partial hops are retained, and gaps, epoch changes and explicit discontinuities reset aubio state. The experiment is not connected to CLI replay or snapshots; silence invalidation, event strength, publication delay, longer-than-32-bit aubio counters, native capture and measured beat accuracy remain integration work. Generated click tests at 44.1/48 kHz check causal bounds, chunking and broad beat intervals, not full-mix accuracy.

## Combined causal music replay prototype

The optional composition feeds the shared mono hops to real aubio tempo and beat trackers and the Essentia tonal tracker. It can replay a path through `FileReader` and emits synchronous beat and overlapping tonal-window callbacks; a live worker can call the same block interface after leaving the audio callback. An explicit interrupt clears internal evidence and requires a new coordinator-issued epoch, but cannot revoke already published UI copies. The snapshot library and actual platform capture backends remain unconnected. Time-aligned windows are not merged chord segments; rolling key is not a validated track-level key; unavailable strengths remain explicitly absent; estimates and strengths remain uncalibrated, and distribution licensing plus real-mix and native qualification are open.

Combined prototype verification on 2026-11-07: `docker build -f tests/spikes/tonal-ci/Dockerfile -t beatglass-music-ci .` and `docker run --rm beatglass-music-ci ctest --test-dir /tmp/build --output-on-failure` passed 57/57 in Ubuntu 24.04 x86-64 with pinned Essentia source and system aubio/libsndfile; the v1 codec option was off. The generated C-major-to-A-minor file test exercises time-aligned windows, causal prefix equivalence, real beat events, sample-rate metadata and source-byte preservation. The default Windows x64 MSVC build with the combined option off passed 52/52 with the codec-enabled development tree. These runs do not qualify the optional Essentia path on Windows, real music, live capture or native GUI behavior.

After reconciling the #26 branch with main at `98be006c97445f987acea8ab16eea57372aaab89`, WSL Ubuntu 22.04 with `BEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON`, slice disabled and pinned Catch2 source reused passed `cmake --build /tmp/beatglass-library-phase1 -j 3` and `ctest --test-dir /tmp/beatglass-library-phase1 --output-on-failure` 32/32. Windows 10.0.26300 x64/MSVC 19.44 with the previously configured static CRT and static SQLite development tree passed `cmake --build C:/Users/zachf/.codex/builds/beatglass-library-static-crt --config Release -j 4` and `ctest --test-dir C:/Users/zachf/.codex/builds/beatglass-library-static-crt -C Release --output-on-failure` 34/34. These are development checks only; WSL is not native Linux qualification and no decoder slice was included.

## Same-source integration proposal

The provisional `ByteSource` and `ByteCursor` establish a borrowed, worker-owned
read-only byte boundary. The existing CLI decoder does not consume it. Secure
snapshot creation, native file identity and decoder integration remain open;
see [same-source proposal](same-source-proposal.md).

## Experimental virtual decoder

A private libsndfile virtual-I/O adapter can synchronously decode a borrowed `ByteSource`; it is not connected to the CLI or cache. It keeps the source borrowed until decoder close, translates callback errors and cancellation to worker-visible outcomes, and is invalidated after mid-read cancellation or failure. Generated WAV, AIFF, FLAC and MP3 tests exercise the adapter on the codec-enabled Windows development build. The byte-source contract was merged in PR #41; snapshot creation, original-file revalidation, secure cleanup, and native platform race tests remain unimplemented. No file is made immutable by this adapter.

Ubuntu 22.04 WSL with GCC 11.4, aubio 0.4.9 and libsndfile 1.0.31 also built this adapter and passed 53/53 CTest cases with `BEATGLASS_TEST_V1_CODECS=OFF`; the virtual codec fixture check covered AIFF there. This is not native desktop qualification, and the older decoder does not qualify MP3 support.

The private virtual decoder now requires an externally issued initial epoch and a strictly increasing epoch for each seek. The process coordinator, not the decoder, must allocate globally unique epochs across reader recreation. The provisional public FileReader borrowed-source constructor and explicit-epoch seek expose this adapter for review; the legacy path-based seek retains local increment behavior. Snapshot creation and publication are still separate responsibilities.

## Offline result contract

PR #57 merged an owned offline result and bounded structural validator for
composite evidence. Actual composite persistence, paged retrieval, native
qualification and a validated track-level key remain open.

## Phase 1 tonal dependency experiment

The standalone [Essentia spike](essentia-spike.md) built the pinned upstream static library and classified two synthetic triads on Linux container and Intel macOS over SSH. It is not integrated into the product, does not qualify musical accuracy or native capture/GUI, and does not resolve Windows MSVC or release-license requirements. Its proposed worker ownership, causal timing and same-open-file cache handoff remain integration work.

## Composite collector and dependency provenance

The experimental music collector stages owned beats and overlapping tonal windows under caller-selected event and label limits. It rejects discontinuities and failures without exposing partial output, and requires caller-observed EOF and source revalidation before publication. Its result has deliberately empty engine, algorithm and configuration identity; cache reuse and composite store publication are disabled. It is not yet connected to the library snapshot worker or a composite store.

An embedded build manifest is a useful starting point only when the build is reproducible and the manifest covers the actual engine, aubio, Essentia, FFT/downmix configuration and relevant transitive artifacts. An embedded digest of the libraries selected at link time does not verify the libraries loaded at runtime: shared libraries can be replaced or resolved differently, and a file hash obtained after loading can race replacement. Static dependencies embedded in a measured immutable executable can avoid this mismatch only if the deployment verifies that exact artifact and its remaining dynamically loaded dependencies. A reviewed platform-specific measurement and deployment policy is required before these identifiers can authorize reuse; version strings or caller-supplied labels alone are insufficient. Until then the collector returns unverified results and must not read or write composite cache entries.

The Windows overlay controller now supports caller-owned persistence of position and opacity, on-screen recovery, and native cursor dragging across mixed-DPI screens. It does not instantiate a DJ UI; the selected/default integration is separate. Windows unit tests cover restoration and immediate persistence after recovery; see [Windows platform status](windows-platform.md) for the prior native evidence and remaining qualification gaps.

After merging main `6aade6e`, the provisional library option was configured with `cmake -S . -B /tmp/beatglass-foundation-review -DBUILD_TESTING=ON -DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`, then `cmake --build /tmp/beatglass-foundation-review -j 1` and `ctest --test-dir /tmp/beatglass-foundation-review --output-on-failure` passed 38/38 on Ubuntu 22.04 WSL. This configuration does not enable desktop, native audio or macOS qualification. CMake emitted an author warning that `SQLite::SQLite3` is deprecated in favor of `SQLite3::SQLite3` in the installed CMake version; host SQLite remains unpinned.

After the concurrent allocator contract merged, the provisional borrowed `FileReader` rejects epoch zero before source access, requires caller-issued increasing seek epochs and rejects its legacy one-argument seek. The caller must keep one process allocator, serialize reader operations, and discard stale publications. With `BEATGLASS_TEST_V1_CODECS=OFF`, WSL `cmake --build /tmp/beatglass-virtual-check -j1` and `ctest --test-dir /tmp/beatglass-virtual-check --output-on-failure` passed 59/59. Windows MSVC codec-enabled `cmake --build C:/Users/zachf/.codex/build/beatglass-phase1-verify --config Debug -- /m:1` and `ctest --test-dir C:/Users/zachf/.codex/build/beatglass-phase1-verify -C Debug --output-on-failure` passed 62/62. The desktop and library prototypes are not yet wired to one process-owned allocator; legacy path-based overloads remain isolated compatibility APIs.

A provisional [runtime dependency identity proposal](dependency-identity-proposal.md) describes Windows and macOS verification candidates and unresolved mapped-image, dependency-closure and configuration checks. Structural result validation and caller-filled strings cannot enable composite cache reuse. No runtime verifier is implemented.

## Bounded file fingerprint review

A separately reviewable, opt-in fingerprint primitive hashes a file using a bounded buffer and supports cooperative cancellation and change detection. The library prototype now exposes worker-side identity construction, collection refresh, explicit digest revalidation, stale-active-result and stale-catalog-row invalidation, missing-source pruning, and republish helpers; republish rechecks before begin and again at the cache commit boundary, preserving old active results when input becomes stale and allowing the same retryable run to commit after the source is restored. Only `ok` fingerprints can become cache identities, while changed/cancelled/error outcomes remain non-cacheable. File replacement, mutation and ABA can evade attribute checks, so the result cannot prove decoder input identity. OpenSSL remains an unpinned development dependency; native product qualification remains open.

The opt-in Windows `LiveMusic` worker experiment drains bounded loopback packets into the same composite engine. It keeps only the latest provisional rolling key and local chord, invalidates via a generation token and numbers updates and clears with a monotonic revision; the UI owner must apply clears and reject older revisions. This is not wired into the desktop, does not establish track-level key or chord segments, and has not been qualified with native Windows Essentia or live audio. It performs no cache access.

The provisional library executor supports retained per-job state, monotonic frame progress and cooperative cancellation, including cancellation of queued work at shutdown. It is a bounded single-worker queue, not a cache or source-identity mechanism. Worker outcomes determine their terminal status so a late stop request does not relabel a successful operation; callers must define their own commit and identity rules.

The independent tracked-executor change at code commit `15aa55be088d4599c24903e7ffffc55de8b0e853` passed 59/59 CTest cases on WSL Ubuntu 22.04 with GCC 11.4 and SQLite 3.37.2. Configuration: `cmake -S /mnt/c/Users/zachf/.codex/worktrees/beatglass-tracked-executor -B /tmp/beatglass-tracked-executor -DBUILD_TESTING=ON -DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON -DFETCHCONTENT_SOURCE_DIR_CATCH2=/tmp/beatglass-slice/_deps/catch2-src`; build and test: `cmake --build /tmp/beatglass-tracked-executor -j1` and `ctest --test-dir /tmp/beatglass-tracked-executor --output-on-failure`. The build was interrupted twice to avoid concurrent resource-heavy work before the successful run. Header coverage and the Docker Sphinx warnings-as-errors plus rendered check passed (72 names). Windows/macOS native builds and hosted CI were not run for this head; CI and Pages deployment are paused at the user's request.

Follow-up code commit `eb0b5f2` passed 63/63 serial WSL Ubuntu 22.04/GCC 11.4 CTest cases with `cmake --build /tmp/beatglass-tracked-executor -j1` and `ctest --test-dir /tmp/beatglass-tracked-executor --output-on-failure`. This covers reentrant destruction and stop callbacks, active-stop ordering, terminal cancellation and late progress. The first strict Sphinx run found two header-comment formatting warnings; after correcting those comments, `python docs/api/check_headers.py` and Docker Sphinx warnings-as-errors plus rendered check passed (72 names). Hosted CI and Pages remain paused, and Windows/macOS runs for this head remain unrun.

The JSON tempo replay now emits causal `beat` records from the real aubio tracker alongside `replay` observations. Each event carries its epoch, event frame and exclusive observed-through boundary; human-readable replay remains tempo-only. Generated-audio integration checks event causality and prefix invariance. This is still provisional beat timing and is not a native capture or musical-accuracy qualification.

## Native evidence handoff reconciliation

The qualification work now has an executable, fail-closed preparation path on merged main. `check_qualification_handoff.py` verifies that the three platform rows link to their platform-specific native backend and desktop/overlay requirements. `verify_evidence_bundle.py` and `check_evidence_readiness.py` validate manifest provenance, canonical retained paths, hashes, required/unique scenario IDs, per-scenario retained evidence and aggregate readiness summaries. These checks establish documentation and evidence integrity only; they do not claim native support.

The remaining Issue #1 exit gate is explicit: perform supported Windows WASAPI, macOS Core Audio and Linux PipeWire desktop/audio/overlay runs, retain build identity, environment, exact commands, logs and screenshots/recordings for every applicable scenario, and obtain an all-platform `READY` report. Headless, CI, WSL, SSH-only, synthetic and portable-fixture results remain preparation evidence rather than qualification.

The readiness report contract is versioned as `readiness_report_schema: 1`; `exit_gate_status` and `exit_gate_reason_codes` are the stable handoff fields. Product support language remains experimental while native qualification is open.

The dependency-light `desktop_cache_bridge_contract` test now checks the complete cache handoff surface—Qt properties, queued worker dispatch, result signal, and visible QML hit state—without claiming a Qt build or native UI qualification.

The dependency-light desktop transition contract now verifies that Library queue progress, retry/cancel controls, asynchronous cache lookup and cache-hit presentation coexist in one UI surface. It does not claim that a Qt build, decoder run or native qualification has occurred.

The desktop storage bridge now exposes an asynchronous cache lookup (`findCached`) and publishes hit/result state only after the worker responds. This keeps SQLite off the GUI thread and gives the future decoder queue a real UI-owned handoff; no decoder or analysis is started by the lookup, and native UI qualification remains pending.

The Library UI now provides a read-only cache lookup action for content, engine, configuration and mode identities. It enables the worker-backed call only when all fields are present; decoder admission and source revalidation remain queue-owned.

Cache hits now render the serialized cached result in the Library surface, making the lookup outcome inspectable instead of status-only. This is intentionally a result handoff, not a claim that the UI has decoded or recomputed the source.

The Library surface now also renders queue outcome and retry availability beside cached results. This keeps a cache hit distinct from queued/running/failed/cancelled analysis state; the existing worker queue remains the owner of decoder execution and retry semantics.

The queue row now includes observed frame progress alongside state and retry availability, giving the UI a source-attributed progress signal without moving decoder or analysis work onto the GUI thread.

Worker progress callbacks are lifetime-safe: queued GUI updates retain only a `QPointer` and are discarded after the bridge is destroyed, so decoder/worker completion cannot publish into a dead UI object.

The worker analysis contract now also hands the verified serialized result to an optional worker-owned publication callback before reporting completion. This is the handoff point for cache/UI consumers; the callback remains off the GUI and audio threads.

`AnalysisJobBridge` now consumes that handoff as a queued `result` property, retaining the worker-thread boundary while making completed analysis inspectable by the desktop surface.

The Library surface renders that worker result separately from cached-result status, preserving the distinction between a newly completed worker result and a cache lookup.

Retrying an analysis attempt clears the prior worker result before the new attempt is queued, preventing stale output from being presented as current work.

Batch analysis now reports how many accepted operations have published verified results through the worker handoff, keeping queue admission counts distinct from completed-result counts.

The published-result count is scoped to the current batch attempt and resets when that batch is retried.

The bridge also exposes the latest verified batch payload for inspection; it is cleared when a new batch or retry begins and remains separate from the single-operation result.

Displayed worker and batch result text is capped at 4096 bytes with an explicit truncation marker; the full serialized value remains owned by the worker publication callback.

Starting a fresh batch also clears any prior single-operation result before queue admission, so the Library surface cannot attribute old output to the new batch.

Fresh batch admission also resets the displayed aggregate frame count, preventing progress from the previous single operation from appearing on the new batch.

Single and batch retries now reset/republish frame progress immediately, rather than waiting for the periodic bridge poll to correct stale values.

Invalid single-operation submissions also clear prior result and frame state before entering `Failed`, so rejected work cannot display stale output.

The desktop bridge now carries the submitted operation source path alongside status, progress, and results, making worker output source-attributed in the Library surface.

Starting a batch clears the prior single-operation source attribution; batch admission remains a multi-source operation and is represented by its batch counters rather than a misleading single path.

The desktop bridge now reports a bounded, source-attributed failure reason for invalid requests and worker failures, and clears it when a new attempt is accepted.

`StorageRecoveryBridge::submitAnalysis` now queues a path-backed decoder through the coordinator-owned worker and reports accepted/rejected submission status to Qt; completion/retry polling remains worker-owned and native qualification is pending.

The same failure explanation is surfaced when any accepted batch operation fails, while successful batch publications continue to report independently.

Batch status now also reports the count of accepted operations that failed, alongside published results and admission counts.

It reports cancelled accepted operations separately from failures, preserving the distinction needed for retry and cancellation feedback.

Batch status also reports accepted operations that reached completion, completing the queued batch outcome breakdown.

The derived queued count identifies accepted operations that have not yet completed, failed, or cancelled; publication is intentionally not subtracted separately because successful publication and completion describe the same terminal operation.

A provisional `make_file_decode_operation` now supplies the worker-side decoder seam: it opens a path with `FileReader`, drains normalized PCM off the GUI/audio threads, checks cancellation and emits a deterministic decoded-frame serialization. Decoder-only operations now pass runner admission and publish through the observable analysis result path; this is still not musical analysis, cache publication, or native codec qualification.

`AnalysisJobBridge::submitFile` now connects that provisional operation to the desktop worker queue with explicit content hash, engine, configuration, and mode identity. Decoder I/O remains off the GUI thread; source hashing, cache admission, musical analysis, and native codec qualification remain open.

Decode progress now flows through the operation’s wrapped progress callback, so the worker handle and desktop bridge observe the same incremental frame counts rather than only the terminal result.

Analysis operations now support an optional stop-aware transactional publisher carrying the run identity and cache revalidation boundary. The runner treats a rejected or cancelled commit as non-complete; legacy non-persistent publication remains supported.

`AnalysisCoordinator` now owns the worker executor and `StorageOwner` together, attaching run-identity publication before queue submission. This is the safe library boundary for a future Qt bridge; native UI wiring and platform qualification remain pending.

`StorageRecoveryBridge::Worker` now owns that coordinator rather than a standalone storage object. Existing cache/retry recovery calls remain queued on the worker thread while sharing the coordinator’s storage lifetime with future analysis submissions.

The desktop recovery panel now presents the asynchronous cache lookup state as an explicit, accessible UI status (`Cached analysis ready` versus an identity miss). This is presentation only: the queue still owns source fingerprinting, admission and decoder execution, and native Qt/audio qualification remains pending.

## Analysis queue admission handoff
`admit_analysis` now gives the future file-analysis queue one worker-owned decision: return a completed cache result without decoding, or reserve a run ID before decoding begins. The helper does not fingerprint or revalidate a source; callers must supply the validated identity and publish only after decoder verification. Desktop scheduling and native qualification remain open.

## Cache acceptance integration
A cache acceptance regression demonstrates that a committed unchanged identity is found on the next analysis request and the analysis callback is not rerun. Changed identity and configuration invalidation remain covered by lifecycle tests; decoder/UI integration and native qualification remain pending.

## Analysis cache handoff
The library now exposes `reuse_cached_operation`, a worker-owned identity lookup that returns a completed cached result before analysis serialization/DSP begins. Configuration/content identity changes miss independently. The caller still owns fingerprinting, source revalidation and scheduling; decoder/UI wiring and native qualification remain open.

StorageOwner now exposes the same completed-result lookup at the storage lifetime boundary, with a regression covering open/lookup/close behavior. This is an integration handoff for the future desktop analysis queue; the GUI still does not schedule real file analysis.
Coordinator admission now returns cache hits without queueing decoder work and reserves a run before submitting misses. The explicit `Submission` result distinguishes cached, reserved, and rejected paths for the desktop lifecycle; retryable persistence remains worker-owned.
Submission now uses coordinator admission: cache hits publish directly to the recovery result without creating a decoder handle, while misses reserve a run and enter the queued lifecycle.
The Library UI now consumes the coordinator admission status and admitted result properties from the recovery bridge, keeping cache-hit and queued-miss identity visible separately.

Library controls now expose coordinator completion status with explicit cancel and retry actions, distinct from cache lookup controls.

The completion surface now renders the coordinator’s decoded result payload separately from cached-result lookup, preserving result identity across retry and cancellation diagnostics.

The Library also surfaces persisted retryable-run count alongside active analysis status, making failed publication/recovery outcomes visible without conflating them with cache hits.

The recovery bridge now displays each persisted retryable run's run ID, content identity, engine, configuration, and mode. Retry resubmission is metadata-gated: the bridge reconstructs a file decoder operation and calls the coordinator resubmission boundary only when an explicit source path and matching identity are supplied; missing metadata is rejected. This is a portable handoff contract, not native qualification.

Recovery guidance now distinguishes unavailable storage from waiting retryable work and a clear recovery state; completed result identity remains a separate field.
A cancellation retry regression now verifies that a cancelled publication leaves no active cache result, preserves the persisted source/decoder snapshot, and can be admitted and committed again with retryable state cleared. This remains provisional until native desktop qualification.
A worker-owned analysis exposes live frame progress through `StorageRecoveryBridge`; cancellation is cooperative and never publishes a stale result. On restart, SQLite restores retryable source/decoder identity and the bridge reports recovery availability, while frame counters correctly restart at zero because they are process-local execution state. A successful retry or explicit discard clears the persisted recovery record and the UI selection.
Persisted recovery selection is reconciled from each worker refresh: retry/discard completion removes resolved IDs, clears the selected run, and updates the retryable lifecycle status. This prevents a completed or discarded run from remaining actionable in the Library UI.
Persisted retry now retains its worker handle after admission, so the same progress, cancellation, terminal status, and refresh path is used for selected-run recovery as for a new submission. A failed or cancelled retry remains visible as recovery work; successful completion refreshes and clears the persisted record.
Analysis failure diagnostics are transient lifecycle state: queued, running, cancelled, completed, and restart-recovery transitions clear the previous error, while a failed attempt keeps its retryable explanation visible until the next transition.
Successful selected-run retry, explicit discard, and cache invalidation clear the shared queue-error diagnostic before refresh. This keeps single-run and adjacent batch/recovery controls from showing an error after the underlying persisted state has been resolved.
Batch status now includes persisted recovery errors when present, while successful retry/discard transitions clear that suffix through the shared bridge error lifecycle. This keeps batch operators aware of recovery work without leaving resolved diagnostics visible.

Windows output endpoint discovery now adopts the native render default on the first successful refresh when the user has not selected an endpoint. A later refresh never overrides an explicit selection; if that endpoint disappears, the bridge clears it and reports selection loss so capture must be reacquired. The portable source contract is checked by python tests/unit/check_windows_endpoint_default_selection_contract.py.
Overlay monitoring controls now return keyboard focus to the live overlay status after a start/stop/retry request, so the resulting Starting, Reacquiring, Tracking, or Error state is immediately available to keyboard and assistive-technology users. Portable coverage is in `python tests/unit/check_overlay_live_feedback_focus_contract.py`.

Overlay capture retry now returns keyboard focus to the live monitoring status after requesting a fresh accepted snapshot, keeping recovery feedback visible without requiring pointer interaction. Portable coverage is in `python tests/unit/check_overlay_capture_retry_focus_contract.py`.

Native overlay retry now returns keyboard focus to the display status after requesting restoration, keeping display recovery feedback available when the overlay remains unavailable. Portable coverage is in `python tests/unit/check_overlay_native_retry_focus_contract.py`.

Hiding the native overlay now returns keyboard focus to the control-window restore action, making the hide/restore recovery loop keyboard-completable. Portable coverage is in `python tests/unit/check_overlay_hide_focus_contract.py`.

Reapplying saved overlay settings now returns keyboard focus to the current-display status, keeping persisted display recovery feedback available without pointer interaction. Portable coverage is in `python tests/unit/check_overlay_reapply_focus_contract.py`.

Unlocking click-through overlay input now returns keyboard focus to the interaction-mode status, keeping the restored input state immediately inspectable. Portable coverage is in `python tests/unit/check_overlay_unlock_focus_contract.py`.

Changing overlay click-through mode now returns keyboard focus to the interaction-mode status, keeping the resulting input state immediately inspectable. Portable coverage is in `python tests/unit/check_overlay_click_focus_contract.py`.

Releasing the overlay opacity slider now returns focus to the interaction-mode status, keeping the final applied overlay setting immediately inspectable. Portable coverage is in `python tests/unit/check_overlay_opacity_focus_contract.py`.

Selecting an overlay size now returns keyboard focus to geometry status after a successful resize, keeping the resulting dimensions immediately inspectable. Portable coverage is in `python tests/unit/check_overlay_size_focus_contract.py`.

Selecting an overlay destination display now returns keyboard focus to current-display status after persistence, while failed selection keeps focus on the selector. Portable coverage is in `python tests/unit/check_overlay_display_focus_contract.py`.

Showing the native overlay now announces the selected-display launch and persisted-settings restore before the control window transitions away. Portable coverage is in `python tests/unit/check_overlay_show_announce_contract.py`.

Restoring the overlay now announces persisted display, geometry, and opacity reapplication before the control window transitions away. Portable coverage is in `python tests/unit/check_overlay_restore_announce_contract.py`.

Hiding the native overlay now announces the control-window fallback transition while retaining overlay settings. Portable coverage is in `python tests/unit/check_overlay_hide_announce_contract.py`.

Quitting Beatglass now announces that overlay display, geometry, size, opacity, and interaction settings are retained before shutdown. Portable coverage is in `python tests/unit/check_overlay_quit_announce_contract.py`.

Failed native overlay launch paths now explicitly state that the control-window fallback preserves overlay settings while recovery remains pending. Portable coverage is in `python tests/unit/check_overlay_fallback_detail_contract.py`.

The overlay actions accessibility description now states that native-launch fallback retains overlay settings, making persistence explicit while recovery is pending. Portable coverage is in `python tests/unit/check_overlay_fallback_status_contract.py`.

The unavailable-overlay actions tooltip now states that the control-window fallback retains overlay settings. Portable coverage is in `python tests/unit/check_overlay_fallback_tooltip_contract.py`.

The overlay actions accessible value now reports that settings are retained during native-overlay fallback. Portable coverage is in `python tests/unit/check_overlay_fallback_value_contract.py`.

The available overlay actions accessibility value now states that persisted settings are retained, alongside the settings revision. Portable coverage is in `python tests/unit/check_overlay_available_value_contract.py`.

Pending overlay recovery descriptions now state that persisted settings remain retained while the control-window fallback is available. Portable coverage is in `python tests/unit/check_overlay_recovery_description_contract.py`.

Persisted overlay geometry status now states settings retention during pending recovery and fallback. Portable coverage is in `python tests/unit/check_overlay_geometry_retention_contract.py`.

Overlay interaction-mode status now states settings retention while recovery is pending. Portable coverage is in `python tests/unit/check_overlay_interaction_retention_contract.py`.

Overlay estimate attribution now states that the selected combined-output source remains retained during pending recovery. Portable coverage is in `python tests/unit/check_overlay_attribution_retention_contract.py`.

Overlay estimate readiness now states that the accepted snapshot remains retained while display recovery is pending. Portable coverage is in `python tests/unit/check_overlay_readiness_retention_contract.py`.

Overlay endpoint status now states that the selected endpoint remains retained during pending recovery. Portable coverage is in `python tests/unit/check_overlay_endpoint_retention_contract.py`.

Overlay live-monitoring status now states that the selected endpoint remains retained while capture recovers. Portable coverage is in `python tests/unit/check_overlay_live_retention_contract.py`.

Overlay controls now expose a dedicated settings-retention status covering display, geometry, opacity, and interaction mode during recovery and fallback. Portable coverage is in `python tests/unit/check_overlay_settings_retention_status_contract.py`.

Overlay settings-retention status is now keyboard-focusable, allowing non-pointer users to inspect persisted recovery state directly. Portable coverage is in `python tests/unit/check_overlay_retention_focus_contract.py`.

Overlay controls now include a “Review retained settings” action that moves focus to the settings-retention status for keyboard recovery review. Portable coverage is in `python tests/unit/check_overlay_settings_retention_review_contract.py`.

The retained-settings review action now has a global `Ctrl+Alt+Shift+S` shortcut that returns focus to overlay recovery context. Portable coverage is in `python tests/unit/check_overlay_retention_shortcut_contract.py`.

In-app overlay shortcut help now documents `Ctrl+Alt+Shift+S` for retained-settings review. Portable coverage is in `python tests/unit/check_overlay_shortcut_help_contract.py`.

The retained-settings review action now changes to “Review recovery settings” with pending-recovery guidance while the native overlay is unavailable. Portable coverage is in `python tests/unit/check_overlay_review_dynamic_contract.py`.

The retained-settings review action now exposes a dynamic accessible value for normal retention versus pending recovery. Portable coverage is in `python tests/unit/check_overlay_review_value_contract.py`.

The retained-settings review action now announces pending versus stable recovery context when focused. Portable coverage is in `python tests/unit/check_overlay_review_announce_contract.py`.

Overlay settings-retention status now exposes the settings revision in its accessible value for recovery diagnostics. Portable coverage is in `python tests/unit/check_overlay_retention_revision_contract.py`.

Retained-settings review announcements now include the current settings revision for recovery diagnostics. Portable coverage is in `python tests/unit/check_overlay_review_revision_contract.py`.

Retained-settings review tooltip now includes the current settings revision for at-a-glance diagnostics. Portable coverage is in `python tests/unit/check_overlay_review_tooltip_contract.py`.

Overlay shortcut help now exposes retained-settings review as an explicit accessible capability. Portable coverage is in `python tests/unit/check_overlay_shortcut_capability_contract.py`.

Shortcut-help accessibility now reports whether retained-settings review is available with the native overlay bridge. Portable coverage is in `python tests/unit/check_overlay_shortcut_state_contract.py`.

Retained-settings review accessibility value now includes the current settings revision. Portable coverage is in `python tests/unit/check_overlay_review_value_revision_contract.py`.

Retained-settings review descriptions now include the current settings revision for screen-reader diagnostics. Portable coverage is in `python tests/unit/check_overlay_review_description_contract.py`.

Overlay settings-retention status accessible name now includes the current settings revision. Portable coverage is in `python tests/unit/check_overlay_retention_name_contract.py`.

Overlay settings summary now surfaces retained-settings state during pending recovery and launch fallback. Portable coverage is in `python tests/unit/check_overlay_summary_retention_contract.py`.

Consolidated overlay settings summary is now keyboard-focusable for direct inspection of display, size, opacity, and recovery state. Portable coverage is in `python tests/unit/check_overlay_summary_focus_contract.py`.

Overlay recovery controls now include a Review overlay summary action for direct inspection of consolidated display, size, opacity, and recovery state. Portable coverage is in `python tests/unit/check_overlay_summary_review_contract.py`.

Reviewing the consolidated overlay summary now announces its settings revision and pending recovery state. Portable coverage is in `python tests/unit/check_overlay_summary_review_announce_contract.py`.

The consolidated overlay summary review action now has a global `Ctrl+Alt+Shift+M` shortcut. Portable coverage is in `python tests/unit/check_overlay_summary_shortcut_contract.py`.

In-app shortcut help now documents Ctrl+Alt+Shift+M for overlay summary review. Portable coverage is in `python tests/unit/check_overlay_summary_help_contract.py`.

Shortcut help now exposes summary-review capability in its accessible value. Portable coverage is in `python tests/unit/check_overlay_summary_help_state_contract.py`.

Overlay summary shortcut announcements now include settings revision and pending recovery state. Portable coverage is in `python tests/unit/check_overlay_summary_shortcut_revision_contract.py`.

Overlay summary review action now exposes recovery state and settings revision in its accessible value. Portable coverage is in `python tests/unit/check_overlay_summary_action_value_contract.py`.

Summary review action descriptions now include the current settings revision. Portable coverage is in `python tests/unit/check_overlay_summary_action_description_contract.py`.

Consolidated overlay settings summary accessible name now includes ready versus recovery-pending state. Portable coverage is in `python tests/unit/check_overlay_summary_name_contract.py`.

The consolidated overlay settings summary tooltip now distinguishes recovery-pending and launch-fallback states. Portable coverage is in `python tests/unit/check_overlay_summary_tooltip_contract.py`.

The overlay summary review action now describes whether recovery is pending or ready alongside its settings revision. Portable coverage is in `python tests/unit/check_overlay_summary_review_description_state_contract.py`.

The overlay monitoring action now exposes the capture-monitor status and recovery-pending state as an accessible value. Portable coverage is in `python tests/unit/check_overlay_monitoring_action_state_contract.py`.

The overlay capture-retry action now exposes retry availability and the settings revision in its accessible value and description. Portable coverage is in `python tests/unit/check_overlay_capture_recovery_action_contract.py`.

Overlay estimate attribution now exposes whether the selected combined-output source is ready or retained during recovery. Portable coverage is in `python tests/unit/check_overlay_attribution_state_contract.py`.

Overlay estimate readiness now exposes accepted-snapshot readiness alongside recovery-pending retention. Portable coverage is in `python tests/unit/check_overlay_readiness_state_contract.py`.

Overlay settings retention status now names its ready versus recovery-pending state alongside the settings revision. Portable coverage is in `python tests/unit/check_overlay_retention_state_name_contract.py`.

Overlay interaction mode now names its ready versus recovery-pending state alongside click-through or interactive mode. Portable coverage is in `python tests/unit/check_overlay_interaction_name_contract.py`.

Overlay capture endpoint status now names ready versus recovery-pending state. Portable coverage is in `python tests/unit/check_overlay_endpoint_name_contract.py`.

The overlay summary review action now names whether recovery is pending or ready. Portable coverage is in `python tests/unit/check_overlay_summary_action_name_contract.py`.

The overlay endpoint refresh action now exposes refresh availability and recovery readiness. Portable coverage is in `python tests/unit/check_overlay_refresh_state_contract.py`.

Overlay live monitoring status now names ready versus recovery-pending state. Portable coverage is in `python tests/unit/check_overlay_live_name_contract.py`.

Overlay estimate attribution now names ready versus recovery-pending state. Portable coverage is in `python tests/unit/check_overlay_attribution_name_contract.py`.

Overlay estimate readiness now names ready versus recovery-pending state. Portable coverage is in `python tests/unit/check_overlay_readiness_name_contract.py`.

Visualization device recovery status now names whether overlay recovery is pending or ready. Portable coverage is in `python tests/unit/check_visualization_device_name_contract.py`.

Visualization recovery announcements now name whether overlay recovery is pending or ready. Portable coverage is in `python tests/unit/check_visualization_announcement_name_contract.py`.

FFT selection readouts now name overlay recovery context. Portable coverage is in `python tests/unit/check_spectrum_readout_name_contract.py`.

Visualization playback feedback now names overlay recovery context. Portable coverage is in `python tests/unit/check_playback_feedback_name_contract.py`.

Playback epoch feedback now names overlay recovery context. Portable coverage is in `python tests/unit/check_playback_epoch_name_contract.py`.

Visualization selection-reset feedback now names overlay recovery context. Portable coverage is in `python tests/unit/check_selection_reset_name_contract.py`.

Visualization capture retry status now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_retry_name_contract.py`.

Visualization data and capture status now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_status_name_contract.py`.

Visualization mode status now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_mode_name_contract.py`.

Visualization mode controls now name overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_mode_controls_name_contract.py`.

Visualization empty state now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_empty_name_contract.py`.

Visualization load state now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_load_name_contract.py`.

Visualization analysis handoff now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_handoff_name_contract.py`.

Visualization interaction hints now name overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_interaction_hint_name_contract.py`.

Visualization clear-selection action now names overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_clear_name_contract.py`.

Visualization clear-selection descriptions now include overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_clear_description_contract.py`.
Visualization mode control descriptions now include overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_mode_description_contract.py`.

Visualization mode control tooltips now include overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_mode_tooltip_contract.py`.

Waveform and FFT mode buttons now name overlay recovery context. Portable coverage is in `python tests/unit/check_visualization_mode_buttons_name_contract.py`.

Waveform and FFT mode button descriptions now include overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_mode_buttons_description_contract.py`.

Waveform and FFT mode button tooltips now include overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_mode_buttons_tooltip_contract.py`.

Visualization clear-selection tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_clear_tooltip_contract.py`.

Visualization interaction hint tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_interaction_tooltip_contract.py`.

Visualization playback feedback tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_playback_tooltip_contract.py`.

Visualization playback epoch tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_epoch_tooltip_contract.py`.

Visualization analysis handoff tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_handoff_tooltip_contract.py`.

Visualization device recovery tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_device_tooltip_contract.py`.

Visualization data/capture status tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_status_tooltip_contract.py`.

Visualization empty-state tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_empty_tooltip_contract.py`.

Visualization load-state tooltip now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_load_tooltip_contract.py`.

Visualization data/capture status description now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_status_description_contract.py`.

Visualization empty-state description now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_empty_description_contract.py`.

Visualization load-state description now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_load_description_contract.py`.

Visualization load-state description now includes overlay recovery readiness. Portable coverage is in `python tests/unit/check_visualization_load_description_contract.py`.

Consolidated overlay settings summary descriptions now distinguish ready, launch-fallback, and recovery-pending states for assistive technology. Portable coverage is in `python tests/unit/check_overlay_summary_description_contract.py`.

## CLI/library evidence update — 2026-11-10

The shipped analysis and music CLIs now document and test help/version entry
points, decoded duration in human and JSON output, and generated WAV/AIFF/
FLAC/MP3 duration consistency without modifying source files. The README and
platform status index these as portable preparation evidence. Native decoder
runtime, packaged dependencies, real playback/output routing, and retained
Windows/macOS/Linux qualification bundles remain open; no headless result in
this update advances those gates.

## Integrated local gate — 2026-11-10

On Windows x64, the isolated `C:\Temp\beatglass-self` checkout at `origin/main`
commit `33b554cd2` configured with `cmake -S . -B build-direct -DBUILD_TESTING=ON`.
`cmake --build build-direct -j 4` completed all configured targets, including
the analysis-runner and Windows endpoint executables. `ctest --test-dir
build-direct --output-on-failure` passed **231/231 tests**; the only non-run
case was the expected unavailable-native-device scenario. This validates the
portable build, unit/integration contracts, documentation checks, and issue-76
QML contract locally. It does not qualify real WASAPI, Core Audio, PipeWire,
desktop overlay, packaging, or native device behavior.

