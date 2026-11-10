# Issue #76 visual implementation audit

## Overlay design evidence

The committed overlay state/component renderings are indexed in the [design
evidence gallery](overlay-design-gallery.md). The gallery includes the ready,
recovery, disconnected, interaction, visualization, and native qualification
matrix renderings; these are review evidence and do not replace native desktop
screenshots on Linux, macOS, or Windows.

## Persisted recovery outcome announcements

The desktop recovery bridge emits terminal `Analysis failed` and `Analysis cancelled` states through the real `analysisStatus` signal. The QML recovery surface announces each selected-run/status pair once, restores focus to Retry, and clears the de-duplication key when the run is cleared or transitions back to a non-terminal state. This prevents repeated worker notifications from producing stale or duplicate spoken feedback while keeping retry available for a genuine new failure or cancellation.

Rapid persisted-run changes are guarded separately: selecting a different run clears the prior outcome key and terminal worker notifications remain silent until that new selector value is explicitly activated. This prevents a late status signal from the previous run from being presented as an outcome for the newly highlighted run.

Selector activation is explicit: programmatic selected-run changes clear outcome/focus state without stealing focus or announcing a terminal result; user activation marks the run eligible for worker outcome feedback, restores selector focus, and instructs keyboard users to Tab into Retry or Discard.

Active-job rejection is isolated from persisted-run recovery: when no persisted run is selected, a real worker `Analysis failed` or `Analysis cancelled` transition is announced once as an admitted Retry action and focus returns to `Retry analysis`. Persisted terminal outcomes use their own run-keyed announcement state, so neither surface duplicates or steals focus for the other.

The active analysis surface now gates admission through the actual worker handle: `retry()` is called only when `canRetry` is true, storage fallback requires `analysisRetryable`, and worker `Failed`/`Cancelled` status transitions announce the available Retry action and restore focus to it. This prevents a stale enabled button from claiming a retry when the worker has already accepted or cleared the job.

Active worker terminal announcements are de-duplicated by status and reset as soon as a retry is admitted or the worker enters a new non-terminal state. A fresh failure or cancellation can therefore announce and restore focus again, while repeated worker notifications for the same terminal state remain silent.

The de-duplication key also resets when the real worker handle reports `canRetry: false`, including close/reopen and new-job admission boundaries. A reopened bridge cannot inherit a prior failure announcement, and a new terminal transition is eligible for fresh announcement and focus restoration.

Storage bridge disconnects are an explicit recovery boundary: `Storage closed` and `Storage error:*` clear active-worker/persisted outcome keys and selection activation, announce that retry is unavailable until reopen, and return focus to `Reopen storage`. Reopening therefore starts with fresh recovery state instead of stale retry affordances.

On reopen, `Opening storage` clears the same keys and marks a pending restoration. The real `retryableRunsChanged` refresh completion then announces the reopened run count, focuses the selector when runs exist, or returns focus to Library with an explicit empty-state announcement.

While `storageReopenPending` is true, the selector, Retry, and Discard controls remain disabled even if a stale binding briefly reports an identity. They become actionable only after the refreshed retryable-run list completes, so keyboard shortcuts cannot act against a disconnected bridge.

The same pending gate covers bulk Discard all: a reconnect cannot open a destructive confirmation against a stale pre-refresh run list. Bulk discard becomes available only after current retryable identities are restored.

Both selected-run and bulk discard dialogs re-check storage availability on acceptance. If the bridge closes while confirmation is open, the destructive request is canceled, the user is told to reopen storage, and focus returns to Reopen storage.

Persisted Retry applies the same click-time check: if storage closes between enablement and activation, no retry request is dispatched; the selected identity is named in the cancellation announcement and focus returns to Reopen storage.

Active Retry now reports the analogous stale-click outcome: if the worker or storage handle loses retryability between enablement and admission, no request is dispatched, the announcement says the worker state changed, and focus returns to Retry analysis for the next valid terminal transition.

Active-worker Discard confirmation re-checks `canRetry` on acceptance. If the worker resolves or changes state while the dialog is open, no discard is dispatched; the user receives an explicit announcement and focus returns to Retry analysis.

The active Cancel control also re-checks the real worker handle at click time. A queued/running transition that resolves before activation produces a no-op announcement and returns focus to Retry analysis; a valid cancel keeps focus on Cancel while the worker reports the terminal transition.

Batch Retry and Cancel use the same click-time status guards: stale terminal/active clicks dispatch no request, announce the current state, and restore focus to the relevant batch recovery control.

Batch Discard confirmation also re-checks terminal status on acceptance. If the batch has moved on while the dialog is open, no discard is dispatched and focus returns to Batch Retry with an explicit stale-state announcement.

The batch discard dialog exposes an explicit accessible Dialog role, announces its non-destructive source-audio boundary on open, and focuses Cancel first so keyboard users must intentionally Tab to the destructive OK action.

Batch Retry and Discard controls expose the live worker status and accepted-item count as accessible values/descriptions, so their enabled state and recovery scope remain truthful while the batch transitions.

Batch Cancel exposes the same live status/count value and switches to an explicit unavailable description after the batch leaves Queued/Running, matching its click-time guard and preventing stale cancellation expectations.

The aggregate Batch analysis queue status now exposes the complete live lifecycle/count string as an accessible value and labels it as live batch lifecycle data, keeping status context available even when individual actions are disabled.

The adjacent Analysis queue progress surface now exposes its live worker status, frame count, retryability, and recovery error as an accessible Status value, with an explicit unavailable description when the bridge is absent.

The C++ storage bridge now resets active analysis retryability, progress, result, and lifecycle status before reopening. Reopen cannot leave a stale active-job Retry enabled while persisted runs are being refreshed; the subsequent worker refresh repopulates only current retryable identities.

Reopen also clears the bridge's prior `analysisReserved` and `analysisAdmissionResult` values before dispatching the open request. Queue admission text is therefore scoped to the prior connection and cannot be mistaken for a fresh reservation after reconnect.

Cached lookup state is reset at the same boundary: `cacheHit` and `cachedResult` are cleared before reopen, preventing a prior database's completed result from appearing current while the replacement storage connection is opening.

Explicit `StorageRecoveryBridge::close()` applies the same cache, result, admission, and retryability reset immediately. Disconnecting storage therefore removes stale completed-result and queue-admission UI before the user chooses Reopen storage.

Retry and discard reset behavior is consistent across Settings and overlay-unavailable recovery: focus returns to the recovery control, historical retry identity and diagnostics clear after success/discard, and a failed retry keeps the current diagnostic visible. This reset contract is portable; native focus timing remains open.

Overlay-unavailable diagnostics are cleared after successful retry or explicit discard: stale run identity and rejection text disappear before the restored overlay state is announced. Failed recovery keeps diagnostics visible and actionable, preserving a truthful fallback path.

After fresh source and display selection, a successful recovery clears the persisted retry rejection and stale identity message before presenting the new accepted state. If recovery fails, the rejection remains visible and actionable; the cleanup boundary is covered portably, while native recovery timing remains open.

When overlay recovery is unavailable, persisted retry rejection remains announced and keyboard-focusable even with reduced motion enabled. The message names the blocked identity/decoder/source condition and points to fresh source and display selection; restoring a compatible source/display clears the stale rejection before retry.

Persisted retry rejection is visible alongside overlay-unavailable/retry states: stale identity, incompatible decoder snapshot, or missing source metadata is reported as a blocked retry, not a successful overlay recovery. The rejection message remains focusable and the user can choose a fresh source/display target; native timing remains open.

Overlay Retry is readiness-gated by an available selected display. The unavailable/display-loss state keeps Retry disabled while no target exists, preserves reduced-motion behavior, and re-enables the action only after display restoration; the accessible value distinguishes “available” from “waiting for a display.”

Overlay Retry is target-specific: it uses the selected display, announces the retry, and returns focus to the display selector after restoration. If no target is available, the unavailable state remains actionable without claiming recovery; native display-loss timing remains open.

Display-loss evidence must show the overlay disappearing, the control window remaining available, an accessible display-unavailable announcement, and Retry succeeding after restoration. The QML fallback restores controls automatically; native monitor loss/recovery, click-through, and multi-display timing still require retained evidence.

Overlay design states are covered by explicit Show, Unlock, Hide, display-change, unavailable-display, empty-display, and Retry controls. Each state keeps recovery guidance accessible and routes display changes through the native overlay bridge; native multi-monitor, click-through, and display-loss evidence remains required.

Hide failure keeps the control window visible, marks recovery pending, and moves keyboard focus to Restore overlay so the user has an immediate recovery action.

Bridge recovery with unavailable persisted settings reactivates the control fallback and focuses Retry overlay, keeping recovery actionable until visual state can be restored.

Theme preview descriptions expose the current reduced/full motion mode alongside palette values, keeping the six-card appearance workflow self-describing for assistive technology.

Theme preview keyboard focus rings now animate only in full-motion mode; reduced motion keeps the ring immediately visible without changing contrast, focus, or selection semantics.

Reduced-motion changes now include the active theme name in their announcement, making combined appearance state verifiable without relying on visual inspection.

Theme changes announce the selected palette together with full/reduced motion mode, while reduced-motion toggles retain the same focus and status semantics.

The six native QML theme palettes now match the design/v1 tokens for surfaces, foregrounds, and accents (Light, Dark, High contrast, Purple, Blue, Yellow); reduced-motion remains an explicit accessible preference and suppresses pulse/transition animation without changing focus or status semantics.

The native close callback delegates through a directly testable QML handler, preserving the same accepted=false, fallback activation, persistence, and focus contract in headless regression tests.

Closing the native overlay window uses the same Hide/Restore fallback workflow: the control window is reactivated, persisted state remains available, and focus returns to Restore overlay.

Display loss reactivates the control-window fallback before focusing display selection, keeping recovery visible and keyboard reachable even when the native overlay was hidden.

Reset appearance restores Light/full-motion defaults, announces the resulting state, and returns focus to the reset control so keyboard users can verify the change.

Paused display recovery reactivates the control window before focusing Resume recovery, so a tray/taskbar fallback remains visible and keyboard reachable after a monitor reconnect.

Native overlay availability loss returns keyboard focus to the visible Retry overlay action after restoring the control window. This makes bridge/display recovery actionable without requiring pointer input; successful retry continues to the persisted-state restore path.
Overlay hide, unlock, display replacement, and retry preserve the reduced-motion contract: announcements and accessible state text remain, while motion is suppressed. Focus returns to a recoverable control after overlay/display changes; native multi-monitor and assistive-technology timing remain qualification gaps.

Endpoint replacement and reacquisition preserve focus on the endpoint selector and route announcements through the shared recovery helper. Reduced-motion mode changes animation presentation only; stale clearing, status text, and keyboard focus remain identical across Settings, DJ, and overlay surfaces.

Endpoint recovery status is consistent across Settings, DJ monitoring, and overlay: endpoint changes announce stale-selection clearing, retry/reacquisition, and fresh accepted snapshots through one helper. Reduced-motion mode suppresses transition animation while preserving the same text and focus semantics; native cross-screen timing remains open.

The complete Settings output-device flow is now traceable across design states: selection announces the chosen endpoint, refresh restores focus to the endpoint selector, permission failure exposes recovery guidance, replacement clears the stale selection, and reacquisition announces the new endpoint. Native device/permission timing remains a qualification gap.

Settings endpoint controls are tied to the real endpoint bridge: Refresh requests a fresh enumeration, permission guidance reflects the bridge status, and device replacement clears the selected endpoint until reacquisition succeeds. Each control remains keyboard reachable and exposes its current state accessibly; native permission/device timing remains open.

Settings-level output guidance covers no-device, permission-unavailable, retry-failure, endpoint refresh, and reacquisition states with matching accessible names and focusable controls. Native readiness still requires evidence of permission prompts, device replacement, and recovery timing on supported systems.

Retry after cancellation or failure now announces that stale results were cleared and fresh evidence is pending, while the retry control remains in the keyboard focus order. The same announcement contract applies when new evidence is admitted; native screen-reader timing remains open.

Recovery controls explicitly participate in tab focus after retry/cancel transitions. Source replacement and endpoint/discontinuity changes clear stale status before replacement work is announced, and the recovery controls remain keyboard reachable; native focus restoration timing still requires assistive-technology evidence.

Source replacement, endpoint switch, discontinuity, and retry transitions expose one accessible recovery narrative: stale labels are cleared before new work is announced, focus remains on the recovery surface, and queued/running status identifies the transition without presenting prior source results as current.

The Library surface now exposes an accessible stale-result status when cancellation or failure invalidates an analysis: the prior result is cleared and retry requires fresh source evidence. The same contract applies after source changes and endpoint/discontinuity transitions; native announcement timing remains a qualification gap.

## Shortcut announcement contract

Shortcut-triggered stop/resume and retry actions must update the same accessible lifecycle announcement used by pointer activation, preserve focus on the active recovery surface, and clear stale result text when an epoch or endpoint changes. The portable contract is checked with `python tests/unit/check_desktop_analysis_transition.py`; native screen-reader announcement timing and focus restoration still require platform evidence.

This is the current implementation audit against `design/v1/DESIGN.md` and `design/v1/STATES.md`. It records what is implemented in the Qt prototype and keeps the remaining design work explicit rather than treating a mock render as a product guarantee.

## Implemented interaction/state slices

- Appearance: theme selection, preview radio semantics, reduced-motion preference, stateful accessible descriptions, compact-header behavior below 760 px, and keyboard-visible focus rings.
- Overlay: show/hide/unlock recovery, click-through state, opacity and size controls, display enumeration, empty-display handling, retry capture/overlay actions, and monitor-dependent control disabling.
- Overlay accessibility: capture, interaction mode, opacity, display, and unavailable-state labels are explicit static status text with dynamic descriptions.
- Visualization: waveform/FFT selection, Ctrl+1/Ctrl+2 shortcuts, pointer and keyboard position selection, Home/End seeking, clear selection, focus ring, bounded frame mapping, generation/revision status, empty/loading/error states, and stale-frame clearing on capture/playback discontinuities.
- Playback: source/epoch-aware presentation clearing, bounded frame seek helpers, state/source/position affordances, and recovery/error presentation.
- Playback accessibility: native state, source, and position status are exposed as static text with explicit names while controls retain state-specific action guidance.

## Explicitly provisional

- The canvas remains a presentation surface. It does not claim calibrated musical truth or imply a seek unless a validated duration/frame mapping is supplied.
- The native overlay, capture, playback, and endpoint bridges still require actual platform qualification. Headless QML tests do not establish native monitor, audio, compositor, or permission behavior.
- Qt QML execution is currently unavailable in this Windows checkout; every QML change records that limitation in its PR.

## Remaining acceptance work

1. Run the QML navigation suite with Qt on a desktop host and capture screenshots at compact and normal widths.
2. Exercise the overlay on monitor removal/re-add, compositor refusal, click-through recovery, and arbitrary-background legibility on macOS and Windows.
3. Connect a validated player duration/frame source before exposing a user-facing seek action from the visualization cursor.
4. Complete native qualification evidence required by issue #1; do not infer completion from this prototype audit.

This audit is intentionally a completion aid, not a claim that issue #76 or issue #1 is closed.

The remaining library detail and analysis status labels now use explicit static-text roles while retaining source-attributed dynamic descriptions.

Settings permission guidance, appearance state, and persistence status are also explicit static text with accessible names.

Generation/revision status is now explicitly identified as static text, keeping accepted presentation lineage discoverable to assistive technology.

Library sorting and the system output-device selector now announce their current values alongside the action guidance.

Playback state/source/position and the active waveform-versus-FFT canvas now include their current values in accessible names, keeping navigation context synchronized with the visual surface.

Overlay capture, interaction mode, opacity, display, and unavailable-state names now include their current values as well, keeping recovery and configuration context synchronized.

Capture/endpoint status and current appearance settings now include their live values in accessible names, completing the remaining dynamic configuration surfaces.

Visualization load/data/selection/generation/capture labels now include their live values in accessible names, keeping the visual status narrative synchronized for assistive technology.

Analysis uncertainty, freshness, progress, batch queue, and persisted recovery names now include their live values, completing the dynamic analysis status surface.

Selected file/source/duration, metadata, and analysis/playback detail names now include live values, keeping the library detail surface synchronized for assistive technology.

Playback selection, visualization mode/data, DJ evidence, and settings persistence names now include live values, completing the remaining cross-screen dynamic status contract.

Library, DJ, and Settings screens now have discoverable Ctrl+Shift+1/2/3 keyboard navigation shortcuts, documented through each tab's accessible description.

Those screen shortcuts now return focus to the selected tab, while existing reduced-motion gates continue to suppress pulse and focus animations when enabled.

All three top-level tabs now share a high-contrast active-focus ring with no motion dependency, keeping keyboard focus visible across themes and reduced-motion settings.

Primary library, playback, capture, and visualization actions explicitly participate in tab focus traversal, preserving keyboard access when controls are conditionally enabled.

Secondary playback-device, capture-endpoint, theme, reduced-motion, and click-through controls now explicitly participate in tab traversal as well.

Theme, playback-device, capture-endpoint, and system-output selectors now expose a consistent ComboBox accessibility role alongside their dynamic names and descriptions.

Primary playback, capture, overlay, and opacity controls now expose explicit Button/Slider roles alongside their stateful names and descriptions.

Reduced-motion and click-through state plus playback format and overlay opacity values are now exposed through explicit accessibility values.

Seek target, playback device, overlay size, and waveform/FFT selected-state values are now explicit accessibility values as well.

Playback state/frame, visualization mode, and current appearance settings now expose explicit accessibility values for state-aware assistive technology output.

Capture, endpoint, overlay interaction, playback-selection, and settings persistence statuses now expose their rendered text as explicit accessibility values across all screens.

Playback error/seek guidance and visualization load/data/selection statuses now expose their rendered values explicitly as well.

Metadata recovery, overlay display availability, and platform permission guidance now expose their rendered recovery text as accessibility values.

Overlay unlock/capture hints and endpoint discovery/recovery guidance now expose explicit recovery values across the affected surfaces.

Playback source, selected visualization data, DJ estimates, and DJ evidence now expose their rendered values explicitly, completing the remaining cross-screen status announcements.

Analysis/playback detail, visualization generation/capture, and DJ monitoring statuses now expose explicit values, closing the remaining dynamic status gaps in the audit.

Analysis uncertainty/freshness, source workflow, and visualization empty-state messages now expose explicit values for assistive technology.

Visualization mode, empty/loading, data, and selection status labels now expose the same static-text role while preserving dynamic view and capture descriptions.

Playback seek limitations, source-selection mismatch, native errors, and overlay recovery hints are likewise explicit status text, keeping recovery guidance discoverable without changing the underlying bridge behavior.

The remaining DJ monitoring, estimates, evidence, and visualization-capture status surfaces now use the same explicit static-text semantics.

Analysis progress, batch queue, persisted recovery, and confirmation text are also exposed as static status text; their dynamic descriptions continue to reflect bridge state.

The library/capture accessibility batch also treats catalog status, endpoint empty/unavailable/recovery states, and selected endpoint status as explicit static text with dynamic descriptions.

Catalog status, result counts, and folder-list navigation now have visual tooltips paired with explicit accessible values and roles.

Current-folder context and empty-library recovery now have dedicated visual tooltips matching their accessible state semantics.

Parent-folder navigation, search, and sort controls now pair explicit roles with visual tooltips for library browsing and recovery.

Folder chooser and open-playback controls now pair explicit button semantics with visual tooltips.

Clear-selection and metadata-retry actions now have contextual visual tooltips matching their accessible recovery descriptions.

Selected source path, duration, and metadata status now have visual tooltips alongside their dynamic accessibility announcements.

The library browser empty/error state now has a visual recovery tooltip alongside its accessible guidance.

Metadata error recovery guidance now has an explicit tooltip label as a visual fallback for users who do not receive accessibility announcements.

Library empty-state text now includes recovery action guidance, and metadata-error guidance includes its current message in the accessible name.

Empty-library folder recovery now includes the live browser state in its accessible description, and duplicate role declarations were removed.

Reopen and clear-source actions now include the current selected source in their accessible descriptions and preserve explicit recovery roles.

The metadata retry action now includes the current error/status text in its accessible description, connecting recovery guidance to the live source state.

Library empty/loading/error announcements now include their current message in the accessible name, while metadata recovery remains explicitly valued and role-annotated.

Library-empty and metadata-error guidance now expose explicit values, preserving actionable source recovery context for assistive technology.

Selected-file name, source, and duration details now expose explicit values for assistive technology.

Metadata retry now declares an explicit button role, completing source-error recovery semantics alongside the existing guidance and status value.

Folder contents now announce selection position and file/folder type, while each browser entry declares a ListItem role for source navigation.

Catalog result counts, current folder, folder-list navigation, sort order, and empty-library actions now expose explicit accessible values and roles.

The library search field and clear-selection action now declare explicit accessible input/button semantics for source browsing and recovery.

Library folder selection, empty-library recovery, and opening the selected playback source now declare explicit button roles alongside their existing keyboard focus behavior.

Native playback sample-rate and channel inputs now declare explicit SpinBox roles alongside their hertz/channel values.

Output capture endpoint selection now exposes its current endpoint value and ComboBox role, completing the transport input semantics.

The no-display overlay empty state now includes actionable recovery guidance in its accessible description, clarifying how to restore display routing.

Seek action, overlay display selection, and system output-device status now expose complete control roles and current values.

Seek target and operating-system output selectors now expose explicit current values and a SpinBox role for frame entry.

Overlay size selection now declares its ComboBox role, and the unavailable-display retry action declares its button role for keyboard and assistive-technology transport.

Overlay display selection and its no-display empty state now expose explicit values, keeping display-routing recovery state available to assistive technology.

Overlay unlock and hide actions now declare explicit button roles, completing transport-control semantics for overlay recovery.

Pause/resume, five-second seek, and playback-device refresh actions now declare explicit button roles alongside their dynamic accessible names.

The stop-playback recovery action now declares its button role explicitly alongside the existing retry, seek, refresh, and overlay-retry names.

Playback availability now includes its current recovery text in the accessible name and value, completing the unavailable-build status path.

Playback seek limitation guidance now includes its current text in the accessible name, making the frame-based recovery constraint discoverable during active playback.

Native playback error announcements now include their current error text in the accessible name, making output recovery failures actionable without requiring visual inspection.

The live overlay tempo, harmony, and opacity status surfaces now expose explicit values and static-text roles so changing monitoring output remains discoverable without relying on visual-only updates.

The overlay header, native playback state, endpoint-empty message, and platform permission guidance now provide explicit values and descriptions, completing the remaining live/monitoring recovery announcements.

Live monitoring guidance and selected-endpoint announcements now expose explicit values and static-text roles, preserving actionable empty-state context when capture is stopped or no endpoint is selected.

The consolidated loading/error audit now asserts visualization loading and empty states, focus affordance, metadata retry, analysis retry/cancel, and batch retry/cancel controls.

The browser/library audit now consolidates folder navigation, delegate semantics, selected-source, metadata, analysis progress, and analysis-waiting states in the static contract.

The consolidated keyboard/accessibility audit now asserts library search, sort, browser navigation, and empty-state keyboard surfaces.

The visualization audit now covers waveform/spectrum shortcuts, focus-ring affordance, overall visualization status, selected-data status, and DJ status/estimate/evidence surfaces alongside the existing canvas and recovery states.

Visualization selection and accepted revision status surfaces are now explicitly asserted in the static visual contract.

Visualization canvas, data, generation, capture, and load-status surfaces are now explicitly asserted in the static visual contract.

Waveform mode controls, visualization mode status, selection status, and selection clearing are now explicitly asserted in the static visual contract.

Overlay opacity control/value and display selection/status surfaces are now explicitly asserted in the static visual contract.

Overlay Escape recovery, unavailable-display, and platform permission guidance surfaces are now explicitly asserted in the static visual contract.

Overlay capture retry, show, unlock, hide, and unavailable-display retry controls are now explicitly asserted in the static visual contract.

Output endpoint empty, unavailable-build, and recovery-guidance states are now explicitly asserted in the static visual contract.

Output-device selection, endpoint refresh, endpoint status, and selected-endpoint surfaces are now explicitly asserted in the static visual contract.

Native playback running/error, playback state/source, and unavailable-build surfaces are now explicitly asserted in the static visual contract.

Native playback start/retry, output refresh, sample-rate, and channel controls are now explicitly asserted in the static visual contract.

Playback reopen, pause/resume, stop, backward-seek, forward-seek, and frame-seek controls are now explicitly asserted in the static visual contract.

Metadata status/retry and analysis uncertainty/freshness surfaces are now explicitly asserted in the static visual contract.

Clear-selection, playback-unavailable recovery, analysis-detail availability, and source-workflow status are now explicitly asserted in the static visual contract.

Selected audio identity, source path, duration, metadata status, and metadata recovery guidance are now explicitly asserted in the static visual contract.

Analysis detail availability, uncertainty, freshness, source workflow, and progress states are now explicitly asserted in the static visual contract.

Analysis batch status, persisted recovery selection/discard, retry, and cancellation controls are now explicitly asserted in the static visual contract.

Playback sample-rate, channel-count, output-device, playback-state, and playback-source controls are now explicitly asserted in the static visual contract.

Library choose-folder, parent-folder, search, sort-order, and empty-state chooser controls are now explicitly asserted in the static visual contract.

Library folder-path, browser-list, empty-state container, and empty-state message surfaces are now explicitly asserted in the static visual contract.

Overlay interaction mode, opacity value, and display status are now explicitly covered in the static visual contract for live presentation state.

Overlay capture, monitor status, monitoring guidance, permission guidance, and selected-endpoint states are now explicitly included in the static accessibility contract.

Output endpoint empty, status, unavailable, recovery, and selected-endpoint surfaces are now explicitly asserted in the visual accessibility contract.

Overlay opacity and display status surfaces are now explicitly included in the static visual contract for display-routing recovery and state reporting.

The visualization empty, data-availability, and capture-status surfaces are now explicitly covered by the static visual contract for waveform/FFT recovery and loading states.

Playback error, position, seek-limitation, source-selection, and unavailable-build states are now explicitly asserted in the visual accessibility contract.

Appearance audit coverage now asserts the reduced-motion switch, reset action, and combined theme/motion status surface remain present and state-bearing.

The primary Library navigation tab now exposes explicit keyboard focus, PageTab role, and hover guidance for accessible navigation.

The appearance reset control now declares an explicit button role, completing semantic coverage for the settings recovery action.

The DJ and Settings navigation tabs retain explicit shortcut/focus-return descriptions in the static contract, ensuring keyboard navigation guidance remains part of the visual accessibility audit.

Live overlay tempo and harmony estimates now expose contextual tooltips, while overlay hide and retry-capture actions explicitly declare button roles for assistive technology.

Overlay unlock guidance and capture status now expose contextual recovery tooltips, while platform permission guidance includes explicit value, role, and description semantics.

Native playback state, unavailable-playback recovery, and first-frame analysis waiting now expose contextual tooltips alongside their accessible status semantics.

Analysis queue capacity, persisted recovery status, retryable-run selection, and discard recovery actions now expose contextual tooltips aligned with their dynamic accessibility semantics.

Selected-source identity, analysis detail/uncertainty/freshness, source workflow, and analysis progress now expose contextual tooltips aligned with their dynamic accessible values.

Capture status, endpoint discovery and selection status, permission guidance, and the monitoring action now expose contextual tooltips that mirror their accessible recovery semantics.

Library folder and audio-file entries now expose context-specific tooltips that match their accessible action descriptions, making directory navigation and source selection discoverable on hover as well as through assistive technology.

Analysis recovery actions now participate in the keyboard tab order explicitly. Retry/cancel controls for single and batch analysis expose matching tooltips and accessible descriptions, so keyboard and pointer users receive the same recovery guidance while jobs are queued, running, cancelled, or failed.

Metadata recovery, source clearing, and waveform/FFT mode controls now explicitly participate in keyboard traversal. Their focusable state is covered alongside the existing error, empty, and loading semantics so recovery remains discoverable without a pointer.

Appearance reset and theme-preview choices now explicitly participate in keyboard traversal as well. Settings recovery and palette selection therefore share the same focus contract as the primary navigation and analysis controls.

The settings output-device surface now exposes a contextual tooltip that mirrors its accessible value, keeping unavailable/default-device guidance discoverable for keyboard, assistive-technology, and pointer users.

Appearance-state and settings-persistence summaries now expose contextual tooltips as well, making theme/motion state and the user-audio safety promise discoverable through the same visual and semantic channel.

Output-endpoint empty, unavailable-build, and recovery guidance now expose dedicated tooltips matching their accessible descriptions, keeping device-discovery recovery actionable across visual and assistive interfaces.

The endpoint selector now exposes a concise tooltip for choosing the live-capture source, aligning the primary settings control with its empty, unavailable, and recovery-state guidance.

Overlay opacity value, display status, and unavailable-display messaging now expose contextual tooltips, completing the visual guidance for overlay routing and native-availability states.

The overlay-display empty state now provides a direct tooltip explaining how to recover when no destination display is present.

Overlay opacity, display, and size labels now expose heading semantics with concise contextual tooltips, keeping the grouped routing controls understandable before the associated values are reached.

The Settings page and output-device subsection now expose heading semantics with contextual guidance, making the purpose of the capture, appearance, and overlay controls discoverable before focus enters each field.

Audio-capture and appearance groups now expose grouping semantics, descriptions, and section-level tooltips; the appearance introduction is also exposed as a static guidance surface for consistent screen-reader and pointer context.

The Settings tab now explicitly participates in tab navigation with a tab role and entry tooltip, while the persistence status exposes an explicit safety description for the user-audio preservation promise.

The DJ tab now has explicit page-tab semantics, keyboard participation, and an entry tooltip; the Beatglass header label is exposed as a heading with workspace context for assistive and pointer users.

The primary navigation bar now exposes toolbar semantics, a concise description, and an entry tooltip so page switching is announced as one coherent navigation surface.

The reduced-motion switch now exposes matching tooltip guidance for its animated-transition preference, keeping the visual affordance aligned with its checkbox semantics and dynamic enabled state.

Theme selection and reduced-motion guidance now reflect their live values in tooltip text, so pointer users receive the same current-state context exposed through accessible values.

The appearance summary tooltip now also reports the current theme and motion mode directly, keeping the persisted state readable without requiring focus on the individual controls.

Reset appearance now exposes the current theme/motion value and dynamically explains exactly which settings will be restored, reducing ambiguity before destructive-looking recovery actions.

The Library page heading now exposes explicit heading semantics and a concise browse tooltip, giving the primary file-navigation surface the same entry context as DJ and Settings.

Library browser column headers now expose column-header semantics and concise name/type tooltips, making the folder listing structure discoverable before users navigate individual entries.

The filtered catalog result summary now exposes an explicit description and current-count tooltip, keeping result-state context available independently of the browser list.

The playback control row is now exposed as one grouped accessible surface with a concise tooltip, giving users context before navigating format, transport, seek, and output-device controls.

Theme preview swatches now expose a grouped semantic surface and entry tooltip, clarifying that the individual radio-style controls are one coordinated palette choice.

Reset appearance now includes the current theme and motion mode in its accessible description as well as its tooltip, keeping recovery context consistent across assistive and pointer interfaces.

Each theme preview swatch now exposes an explicit selected/not-selected accessible value in addition to its radio-button description, making palette state unambiguous to assistive technology.

Duplicate catalog-status and output-device value declarations were consolidated so each surface has one authoritative accessible/tooltip value, avoiding conflicting announcements.

The separate filtered-result count now has a distinct `catalogResultCount` object identity, preventing duplicate automation targets while preserving the primary catalog-status contract.

Redundant text labels inside each folder-list delegate are now excluded from the accessibility tree; the delegate’s richer list-item name, value, and action description remain the single announcement surface.

The browser empty-state container now exposes grouping semantics and recovery guidance, so its message and chooser action are announced as one coherent unavailable-state surface.

The folder browser frame and overlay action row now expose grouping semantics with concise entry tooltips, giving users context before navigating list items or show/unlock/hide actions.

Overlay interaction/opacity and display/size rows now expose separate grouping semantics and entry tooltips, clarifying the two distinct routing-control clusters before focus reaches their individual fields.

Playback and overlay control groups now expose live state values in addition to static descriptions, so assistive technology and tooltips can communicate availability, playback state, interaction mode, opacity, and destination display together.

Live monitoring and visualization-mode rows now expose grouped semantics and live values, giving users one coherent context surface before reaching start/stop or waveform/FFT controls.

The live visualization canvas now exposes its current mode and selected-position value directly, aligning keyboard selection feedback with the rendered waveform/FFT surface.

DJ monitoring status, estimates, evidence quality, and monitoring guidance now expose contextual tooltips, making live-state interpretation and recovery guidance consistent across the visualization and overlay surfaces.

The DJ page heading and live-output monitoring summary now expose explicit heading semantics and a concise state tooltip, making the monitoring context discoverable before users reach the action controls.

The live monitoring state and guidance are now exposed as one grouped surface, while the DJ status, estimates, and evidence-quality message share a second grouped surface. Both groups report the current monitoring state and provide entry tooltips, so assistive technology users can understand capture readiness and provisional evidence before navigating individual labels.

Visualization empty, load, data-count, selection, generation, and capture messages now share a grouped status surface with a live ready/waiting value and entry tooltip. This keeps the visualization’s changing data lifecycle coherent for assistive technology users while preserving each detailed status label.

The visualization position status and clear action now share a selection group with a live percentage-or-none value and reset tooltip, making the keyboard/canvas selection lifecycle discoverable as one control cluster.

The Library’s selected-audio panel now exposes one grouped details surface covering source identity, metadata status, recovery actions, analysis, and playback context. Its value follows the selected file and metadata state, giving assistive technology users a stable entry point for the whole source workflow.

Selected-audio recovery actions now expose live values: clearing reports the selected filename or “no selection,” and metadata retry reports the current metadata error/status. This makes action availability and recovery context explicit before activation.

The selected-audio details column now sits inside an explicitly named panel grouping with its own entry tooltip, preserving the visual card boundary as a meaningful navigation boundary for assistive technology.

The overlay’s live evidence surface now exposes one grouped semantic boundary for its header state, tempo/harmony estimates, capture status, recovery hint, and retry action. The group reports the current capture state and provides a single entry tooltip before users navigate its dynamic evidence.

The live visualization canvas now declares graphic semantics in addition to its mode/position value, and its empty state provides a direct availability/next-action tooltip. This aligns the interactive rendered surface and its no-data state for keyboard and assistive navigation.

The live visualization canvas now supports Escape as a keyboard recovery action that clears the selected position, and both its accessible description and tooltip document that path alongside Left/Right/Home/End navigation. This keeps selection state reversible without requiring pointer input or a separate button.

Waveform and FFT mode controls now expose explicit radio-button roles, while the clear-position action reports whether it is available or has no selection to clear. Together with the grouped selection surface, this makes the visualization’s mode and position interaction state explicit before activation.

Primary navigation now reports the current Library, DJ, or Settings page as a live toolbar value, and its tooltip includes the same current-page context. This gives keyboard and assistive navigation a stable location announcement while switching between the three product surfaces.

The Settings tab now uses the same explicit page-tab role as Library and DJ, keeping the primary navigation contract consistent across all three destinations for keyboard and assistive technology users.

All three primary navigation tabs now report a selected/not-selected value in addition to the toolbar’s current-page value, so users can understand both the active destination and each tab’s state while moving through the navigation.

The primary navigation group now documents all three page-switch shortcuts in its accessible description, keeping keyboard-discoverable navigation available even when focus is on the toolbar as a whole.

Each primary navigation tab now updates its tooltip to identify the selected destination when active, keeping pointer and keyboard hover/focus feedback aligned with the live selected/not-selected values.

The Settings Appearance group now reports its current theme and motion mode as a live group value, keeping the visual preference cluster’s state available before users navigate its individual controls.

The Appearance group tooltip now includes the current theme and motion mode, keeping the grouped settings context synchronized for pointer and keyboard users.

The Appearance group description now carries the same current theme and motion state as its value and tooltip, so assistive technology receives consistent context at group entry.

Theme preview swatches now support Left/Right keyboard navigation while focused, applying the next palette and synchronizing the toolbar selector and persisted preference. This gives the compact preview control a complete keyboard interaction path in addition to direct activation.

Home and End now jump directly to the first and last theme swatches, completing predictable keyboard traversal for the palette preview group while preserving selector and preference synchronization.

Arrow and Home/End palette navigation now moves focus to the newly selected swatch, preventing repeated key presses from operating on a stale delegate and keeping the visible focus ring aligned with the active theme.

The contract checks now cover focus movement for both relative arrow navigation and the Home/End boundary jumps, protecting the complete keyboard palette traversal path.

Escape now returns focus from a theme preview swatch to the main theme selector, providing a predictable keyboard exit from the swatch cluster without changing the selected palette.

Escape now also returns focus from either Waveform/FFT mode button to the live visualization canvas, keeping keyboard navigation flowing from mode choice into the interactive rendered surface without pointer input.

The live canvas now supports Page Up/Page Down for coarse ten-percent position movement alongside fine Left/Right steps and Home/End boundaries. The accessible description, tooltip, and contract checks document this complete keyboard navigation vocabulary.

All canvas navigation keys now explicitly consume their events after updating selection, preventing parent controls from also handling the same keystroke and keeping keyboard interaction scoped to the focused visualization.

Enter/Return on a selected canvas position now moves focus directly to the Clear position action, while Escape still clears immediately. The canvas description and tooltip document both recovery paths for keyboard users.

Space now provides the same focus path as Enter/Return from the selected canvas position, covering the conventional activation key alongside the explicit clear and Escape recovery actions.

Ctrl+Left and Ctrl+Right now jump directly to the visualization boundaries, complementing fine, coarse, and Home/End movement for efficient keyboard scrubbing. The canvas announcement and tooltip include the new shortcuts.

Ctrl+Up and Ctrl+Down now switch directly between waveform and FFT modes from the focused canvas, returning focus to the selected mode control so the mode change remains visible and reversible through the keyboard.

Shift+Left and Shift+Right now provide five-percent position steps between fine arrows and Page Up/Page Down coarse movement. The canvas announcement and tooltip expose the full scrubbing scale.

Pointer interaction now restores canvas focus after clicking, and the canvas supports wheel scrubbing in one-percent increments with bounded selection. The tooltip and contract checks document both pointer recovery and scroll interaction.

Holding Shift while scrolling now increases wheel scrubbing to five-percent steps, matching the keyboard’s medium-step interaction and giving pointer users the same fine/coarse control range.

Right-clicking the canvas now clears the selected position and restores canvas focus, providing a direct pointer recovery action that is documented alongside the keyboard clear paths.

Ctrl-modified wheel input now provides ten-percent scrubbing, extending the pointer fine/medium/coarse scale (plain, Shift, Ctrl) to match the canvas keyboard controls.

Double-clicking the canvas now clears the selected position and restores focus, complementing right-click and keyboard recovery paths for pointer users who prefer a gesture over a context action.

All keyboard and pointer clear paths now share one recovery helper that clears the selection, repaints the canvas, and restores canvas focus. This keeps Escape, Enter/Space, right-click, double-click, and the Clear position button behavior consistent.

The issue-76 contract suite now verifies the complete visualization interaction matrix as a set: keyboard clear/focus paths (Escape, Enter, Space), pointer clear paths (right-click, double-click), wheel scrubbing, and pointer-focus restoration. This prevents a future change from preserving one input modality while silently regressing another.

The Clear position action now announces all equivalent recovery paths in its accessible description and selected/empty state in its tooltip, connecting the focused action to the broader keyboard and pointer interaction matrix.

The visualization now renders a live interaction hint below the canvas: empty state guidance explains pointer/keyboard selection, while an active selection announces its percentage and clear gestures. This gives sighted and assistive users the same immediate recovery context.

The interaction hint now follows the active Waveform/FFT mode at runtime, naming the exact surface users are selecting or clearing. This keeps the responsive hint aligned with mode changes from buttons, shortcuts, or Ctrl+Up/Ctrl+Down canvas navigation.

The Waveform and FFT controls now expose selected-state descriptions directly, so the mode controls and the responsive interaction hint announce the same active visualization state before focus moves into the canvas.

The visualization canvas and interaction hint now adapt to compact-header layout: the canvas reduces its preferred height and the hint scales its text while retaining wrapping. This keeps the runtime guidance usable in narrow desktop layouts without hiding the interaction contract.

The interaction hint now adopts the error color when capture is in an Error state and the active theme foreground otherwise; the status stack also tightens spacing in compact layouts. Runtime recovery state and responsive density therefore remain visually legible together.

When capture enters Error, the interaction hint now changes its message to “retry monitoring before selecting” the active mode, preventing users from attempting selection against an unavailable stream. The same runtime state drives the error color and capture-status labels.

The error-state visualization now exposes a contextual Retry monitoring button when the capture bridge can restart. Activating it starts capture and returns focus to the canvas, closing the loop between runtime error guidance and recovery behavior.

Starting and Reacquiring states now render a dedicated recovery-feedback label below the retry action, so the transition after activation remains visible and accessible until capture returns to a usable state.

The grouped visualization status surface now reports capture error, Starting, Reacquiring, ready, or waiting as its live value, and its tooltip reflects the same state. Assistive users can therefore distinguish data absence from an active recovery transition at the group boundary.

The issue-76 contract suite now checks each runtime status branch explicitly—Error, Starting/Reacquiring, ready data, and waiting for data—so future state changes cannot silently drop a visible or accessible transition.

The visualization status group now includes a state-aware Clear selection action. It is enabled only when a waveform or FFT position is selected, reports available/unavailable through its accessible value, and keeps keyboard and pointer recovery paths available from the same responsive status layout.

The clear action now adapts its label and width in compact headers, while its accessible description and tooltip include the current capture transition. This keeps the action understandable while Starting, Reacquiring, Error, or steady-state monitoring changes around it.

The parent visualization status group now carries the selection-clear availability alongside capture and data state, so assistive navigation announces both the live runtime condition and whether an actionable selection exists.

The compact-header theme preview group now reports the selected theme directly and includes it in its entry tooltip, preserving palette context when users navigate swatches without the full appearance controls visible.

Each theme preview swatch now updates its tooltip when selected, explicitly identifying the active palette instead of repeating a generic selection action. This keeps pointer and keyboard feedback aligned with the radio-button value.

The Theme previews group’s accessible description now carries the current palette alongside its selected-swatch guidance, keeping the non-visual group announcement synchronized with the dynamic value and tooltip.

Starting/Reacquiring feedback now reserves a responsive line height, keeping recovery guidance readable while the status group changes around it.

Retry monitoring now reserves a compact/full minimum width, keeping its recovery label intact while adjacent guidance wraps.

Starting/Reacquiring feedback explicitly permits zero minimum width, allowing its guidance to wrap within the responsive status column rather than expanding the surrounding layout.

Capture-status guidance now also permits zero minimum width, keeping stopped/error instructions inside the responsive visualization column.

The empty-state message likewise permits zero minimum width, allowing its error, stopped, and waiting guidance to wrap without expanding the visualization layout.

Load-state evidence now permits zero minimum width as well, keeping Unavailable/Ready/Waiting/Idle text within the responsive status column.

Capture transitions now expose a visible and accessible reset explanation when selection is cleared: stopped, starting, reacquiring, and error states each identify why the previous visualization position is no longer available.

The waveform and FFT canvas now consumes the active six-theme palette at render time: the background follows the theme surface, plotted data uses the theme accent, and the selection cursor uses the theme foreground for contrast. Theme changes therefore affect the live visualization rather than only surrounding controls.

The canvas also draws a low-contrast, foreground-derived three-row grid and resets its alpha before rendering data, giving each theme a consistent orientation cue without overpowering waveform, FFT bars, or the selection cursor.

In compact headers the grid reduces to one center guide, matching the shorter canvas height and preserving data area for waveform/FFT content; full layouts retain the three-row grid.

External theme preference changes now explicitly request a visualization repaint after updating the selected theme token, keeping the canvas synchronized when settings are restored or changed from another surface.

The visualization keyboard focus ring now uses the active theme accent token, keeping focus visibility aligned with the selected palette instead of a fixed blue highlight.

Waveform and FFT mode controls now use the same theme accent for active focus and the theme foreground for their resting border, keeping keyboard mode navigation visually consistent with the canvas focus ring across all six palettes.

Selected waveform and FFT controls now use a derived darker accent fill, while unselected controls use the active theme surface. This preserves mode hierarchy and contrast as the user changes palettes.

The mode status label now follows the theme foreground and compact-header typography tokens, keeping the current waveform/FFT context legible in both full and narrow layouts.

The empty visualization state now uses the same compact typography and foreground token as the mode status, with an explicit error tint when capture fails, keeping next-step guidance legible across all six themes.

Starting and Reacquiring recovery feedback now uses the active theme accent and compact typography token, visually separating an active transition from the steady empty state while remaining consistent across screen sizes.

The visualization Retry monitoring action now uses the active theme accent as its button surface with the theme background as text, giving the recovery affordance a consistent high-salience treatment across palettes.

The Clear selection action now uses the active theme foreground as its surface and theme background as its label, with reduced opacity when unavailable, aligning selection recovery with the retry control while preserving disabled-state clarity.

Selection-reset explanations now use the active theme accent and compact typography, keeping transition feedback visually prominent without introducing a new one-off color token.

Load-state and data-status labels now consume the theme foreground and compact typography tokens, keeping runtime evidence readable as the visualization moves between unavailable, waiting, and populated states.

Waveform/FFT sample totals and selected-view summaries now use the same tokens, completing the responsive visual hierarchy for runtime evidence and preventing a palette change from leaving secondary status text behind.

The selected-position status now follows the same foreground and compact typography tokens, keeping the active scrub position legible beside its clear action in both full and compact layouts.

The inline Clear position action now shares the themed surface, label contrast, and disabled opacity treatment used by the primary visualization clear action.

Generation and revision status now use the theme foreground and compact typography tokens, keeping provenance evidence legible beside the live data state on narrow screens.

Capture-status guidance now uses the theme foreground in normal operation and the existing error tint when capture fails, with compact typography for narrow layouts.

The selection status/action row now reduces spacing in compact headers, preserving room for both the current position and its reset affordance without changing keyboard order.

Generation/revision evidence now wraps within the available width, keeping long provenance values readable instead of truncating them on compact screens.

Selected-position status now fills the available row width and wraps in compact layouts, preserving the status/action relationship without clipping the current selection announcement.

Waveform sample and FFT-bin counts now wrap within their responsive status width, preventing long runtime evidence from forcing horizontal crowding in compact headers.

Selected-view sample/bin summaries now wrap as well, preserving mode-specific evidence when compact layouts narrow the visualization column.

Selected-view summaries reserve responsive line height for wrapped text, keeping mode-specific context separated from the selection controls.

The live waveform/FFT data-status label now permits zero minimum width, keeping its sample/bin evidence inside the responsive visualization column.

The live data-status message reserves responsive line height, keeping populated and waiting states visually stable as their evidence wraps.

The stacked visualization status group stays top-aligned as labels wrap, keeping the evidence hierarchy anchored while the surrounding responsive layout changes height.

Selection controls now include a bounded Seek to position action. When playback is active and waveform samples are available, the selected normalized position is converted to a source frame through the existing epoch-aware playback seek contract; the action remains unavailable without those real runtime prerequisites.

Playback change notifications now synchronize the visualization selection from the live playback frame over the available waveform span, so the selected cursor follows actual source playback while preserving the existing clear-on-source/epoch transitions.

Canvas rendering now requires a valid presentation generation and revision before consuming waveform/FFT arrays. Reduced-motion remains a presentation preference for animated surfaces only; it does not disable the real analysis data path or static waveform/FFT rendering.

The renderer now validates waveform point pairs and finite FFT magnitudes before issuing canvas geometry, so malformed analysis results are skipped without breaking the rest of an accepted generation.

The status group now exposes an analysis handoff label that reports the accepted generation and revision consumed by rendering, or explicitly waits for an accepted result. This makes the queue-to-visualization boundary visible while retaining generation gating.

The canvas now records the revision it last rendered and updates that value on each accepted presentation change before requesting a repaint; assistive canvas state therefore reflects the actual handoff revision driving the visible waveform/FFT.

Capture, source, and epoch resets now clear the rendered revision alongside the presentation arrays, and invalid revisions are represented as -1 until a fresh accepted result arrives. Stale handoff metadata therefore cannot outlive its source data.

Playback source and epoch transitions now provide explicit stale-clear explanations, connecting the actual playback bridge invalidation path to the visible selection-reset feedback.

The analysis-handoff label now carries that stale-clear reason while waiting for replacement analysis, so source/epoch invalidation cannot silently appear as an ordinary empty state or allow old handoff context to look current.

When a valid replacement revision arrives, the presentation change handler clears the stale-replacement message, updates the rendered revision, and requests the repaint in the same accepted-result callback.

Accepted waveform and FFT arrays are now snapshotted into canvas-local rendered buffers before repaint; invalid revisions clear both buffers. Rendering therefore consumes the accepted handoff snapshot rather than reading mutable bridge arrays after invalidation.

Playback selection synchronization and Seek to position now use the same canvas-local waveform snapshot, and stale-reset lifecycle clears it explicitly before presentation invalidation. Seek and cursor updates therefore cannot target superseded bridge arrays.

The visualization status group now exposes the queued/accepted analysis handoff alongside the generation-gated presentation. An accepted result identity combines the selected source, generation, and revision; seek availability is gated on that identity as well as the local waveform snapshot, making it explicit which accepted result selection controls target.

That identity is now withheld until the selected Library analysis reports a completed/result-ready handoff. Canvas pointer and wheel selection are gated by the same completion contract, so an in-flight or unrelated presentation cannot create a seek target before the selected source has a usable result.

The visualization now reports its reduced-motion state next to the accepted snapshot. Reduced motion affects animated presentation surfaces only; the accepted waveform and FFT arrays remain static, source-backed selection targets and continue to support playback seek.

Load and playback feedback now carry that contract through recovery states: a ready reduced-motion canvas says it is using a static snapshot, and selection feedback distinguishes an accepted-snapshot seek from the unavailable state before playback starts.

The playback bridge helpers now enforce the same accepted-result identity and presentation-ready gates as the controls. Selection sync and seek therefore return without mutating playback when a snapshot is stale, incomplete, or no longer belongs to the selected Library result.

Seek availability now also requires live playback source/epoch alignment. The visualization reports the current epoch when aligned and carries the explicit source/discontinuity reset reason after a clear, so stale selections cannot silently remain actionable.

Accepted snapshots now record the playback epoch used when they were published. The playback bridge change path compares that epoch before synchronizing selection and clears the local waveform/FFT snapshot plus selection on discontinuity, ensuring stale data cannot survive an epoch transition.

Snapshots also record the playback source URL. Source changes and capture-state transitions now clear that source-bound snapshot and selection through the same recovery helper, so a new source cannot inherit a prior source’s cursor or seek target.

The native playback bridge now publishes dedicated source, epoch, and state notifications in addition to its aggregate change signal. QML therefore receives the actual transition events needed to invalidate visualization snapshots, including source replacement, seek epochs, stop, and pause/resume state changes.

The focused bridge contract test checks those notify properties, signal declarations, stop/pause/seek emission paths, and matching QML reset handlers. This keeps source and discontinuity invalidation covered even when a full Qt build is unavailable.

The visualization recovery row now exposes direct Resume and Stop playback actions. Resume is available only while paused and preserves the accepted snapshot; Stop invokes the real bridge stop path, which clears the snapshot and announces the recovery reason through the existing accessible feedback.

The same actions now have application-level keyboard parity: Ctrl+Alt+R resumes paused playback, and Ctrl+Alt+Shift+S stops active playback. Shortcut enabled states mirror the buttons, so keyboard users receive the same state and stale-snapshot behavior as pointer users.

The shortcut contract is executable: the focused regression verifies each sequence, enabled-state predicate, bridge action, and matching visualization button action, preventing keyboard and pointer recovery paths from drifting apart.

Shortcut and pointer recovery actions now publish the same live accessible status label. Resume announces that the accepted snapshot was retained; Stop announces that it was cleared, making keyboard-triggered state changes observable to assistive technology.

The announcement lifecycle now also updates on analysis cancellation, endpoint changes, and capture/source resets. Each transition explains that the current snapshot is stale or awaiting replacement, so recovery feedback cannot remain stuck on an earlier playback action.

Recovery announcements now share a helper that restores focus to the visualization canvas. Cancellation, endpoint transitions, source/discontinuity clears, and playback recovery therefore leave keyboard users at the next actionable visualization target while preserving the same status wording.

Retry controls now invoke the real analysis/recovery bridge through one helper. The UI announces queued admission while leaving focus on the retry control, then restores visualization focus only after the bridge reports a fresh completed result and the accepted identity is ready.

The visualization status group now exposes explicit no-device, capture-permission/retry-failure, and reacquisition states from the live endpoint and capture bridges. Error and reacquiring transitions announce the recovery guidance and restore focus to the visualization target.

Settings output-device controls now announce endpoint selection and refresh requests, keep focus on the endpoint recovery controls, and expose a live retry-failure message that calls out device permission and reconnect guidance.

The selection status/action row now shares top alignment with the evidence group, keeping wrapped status text and its clear action anchored at the same visual edge.

Generation/revision provenance now wraps explicitly alongside its shrinkable width, keeping long identifiers readable in compact layouts.

The selection status/action row now uses vertical-center alignment, keeping wrapped status text and its clear button visually balanced as the compact layout changes height.

The inline clear action now reserves a compact/full minimum width, preventing its label from collapsing while leaving the neighboring selection status room to wrap.

The selected-position label explicitly permits zero minimum width, allowing the row layout to prioritize the clear action while wrapping status text on narrow screens.

The canvas focus ring reduces from three to two pixels in compact headers, preserving a clear keyboard-focus cue while reclaiming a small amount of visualization space.

The selected-position cursor similarly reduces from two to one pixel in compact headers, retaining the theme-foreground cue without overpowering the smaller canvas.

The canvas now also has explicit compact/full minimum heights, preserving a usable pointer and keyboard interaction target when the surrounding screen is constrained.

The visualization status group explicitly permits zero minimum width, allowing its evidence labels to wrap instead of forcing the surrounding screen wider under responsive pressure.

The interaction hint reserves a taller compact-layout line box for wrapped guidance, preventing keyboard/pointer instructions from colliding with adjacent recovery controls.

Visualization status, data counts, and selected-view readouts now consume the canvas-local accepted waveform/FFT snapshots used by rendering and seek. The visible evidence therefore clears and updates atomically with stale-result lifecycle instead of reading mutable bridge arrays.

Appearance settings now publish live accessible announcements when a theme is restored or reduced-motion changes. Reduced-motion messaging explicitly confirms that static accepted visualization snapshots remain available, keeping settings state aligned with runtime rendering behavior.

Overlay display selection now persists in the desktop session: when the screen list returns and the saved index is still valid, QML reselects that target and announces readiness; if it is absent, explicit target selection remains required.

Overlay click-through/unlock and opacity changes now flow through the same live recovery announcement path. Click-through explicitly explains how to restore interaction, unlock announces restored keyboard/pointer access, and opacity changes report the applied percentage.

Overlay size selection now persists within the desktop session and announces the applied Compact/Medium/Large state. When the native overlay is unavailable, an explicit tray/taskbar fallback status keeps the control window available for recovery and retry.

Endpoint bridge transitions now actively clear source-bound visualization selection, distinguish retry failure from no-device reacquisition, and restore focus to the endpoint selector. A replacement accepted snapshot is therefore required after endpoint replacement rather than reusing stale waveform/FFT state.

Settings endpoint, DJ monitoring, and overlay availability now share the same capture-recovery reset path and no-device/reacquisition language. A failed monitor or unavailable overlay clears stale visualization selection and returns focus to the recovery target.

Overlay hide, unlock, and display changes now use that same helper. Their announcements and focus restoration remain immediate even with reduced motion enabled, while any output reacquisition still requires a fresh accepted snapshot.

Overlay show and screen-list transitions use the same fallback contract: a missing display restores control-window guidance, while a restored display announces that a target must be chosen before overlay monitoring resumes.

Retry overlay is now disabled until a display target exists. Once a display returns, the explicit retry action announces its attempt and keeps the control window available until the native overlay actually restores.

After display restoration, Retry remains disabled until the user explicitly chooses a target. Selection clears that gate, restores focus to Retry, and announces readiness; reduced motion does not alter this keyboard/focus contract.

When the overlay is unavailable, the recovery panel may show the persisted admission/rejection bridge result only for failed or cancelled analysis, explicitly labeling it historical rather than current visualization data. This prevents retry diagnostics from making stale-result claims.

That historical diagnostic is dismissed when retry or discard is invoked, and restored only when a subsequent admission reports a new failed/cancelled result. Successful recovery therefore cannot leave old persisted rejection text visible.

Invalid persisted opacity is covered as a distinct visible recovery state: restore applies the safe 90% fallback and announces the corrected value before any overlay relaunch.

Invalid persisted position is a separate visible recovery state: restore rejects non-finite native coordinates, keeps the control-window fallback visible, and names the saved display in the recovery announcement instead of presenting an unpositioned overlay.

Invalid persisted size is also a separate visible recovery state: restore rejects non-finite native width or height, keeps the control-window fallback visible, and announces that the saved overlay size is unavailable before any native presentation.

Capture loss is a distinct stale-source state: Error and Reacquiring mark the last tempo/harmony values stale in their accessible values and descriptions until Monitoring returns, while the recovery announcement directs the user to retry for a fresh accepted snapshot.

When Monitoring returns after that stale state, the overlay announces that a fresh accepted snapshot is available and restores focus to the live monitoring status, completing the keyboard recovery loop without requiring pointer input.

Entering the capture Error state now moves keyboard focus directly to Retry overlay capture after announcing the missing device or permission, so the failure state is actionable without pointer navigation.

The intermediate Reacquiring state also moves focus to the live monitoring status while retaining the stale-evidence marker, so keyboard users can follow recovery before Monitoring publishes a fresh accepted snapshot.

The capture status itself now exposes the stale-evidence marker as an accessible value and description, keeping the recovery state discoverable even when users do not navigate to the tempo or harmony readouts.

The live monitoring focus target repeats that stale-evidence context during Reacquiring, so keyboard users receive the same source-freshness explanation at the point where recovery focus is restored.

Opacity reset now rechecks its in-flight marker at activation time, rejecting duplicate native setter requests while a previous reset is still awaiting its bridge echo.

Native geometry announcements now validate finite coordinates and both dimensions before claiming restoration; malformed snapshots keep geometry recovery pending and direct users to display recovery.

Explicit Stop overlay monitoring now marks the last tempo/harmony snapshot stale immediately; restarting monitoring must return to Monitoring before those values are presented as current again.

When the native overlay is available but no capture bridge is connected at startup, the overlay marks displayed estimates stale and announces that monitoring must connect before fresh evidence can be presented.

The Start/Stop overlay monitoring action now repeats the stale-evidence gate in its accessible value, keeping the required next action explicit while monitoring is stopped or reconnecting.

Retry overlay capture repeats the same stale-evidence context in its accessible value and description, making the recovery action self-describing when capture has failed.

The grouped live-evidence surface also reports stale capture context before its child controls, so screen-reader navigation announces freshness requirements at the recovery section boundary.

Capture Starting is now an explicit overlay recovery transition: it marks prior live evidence stale, announces that fresh audio is pending, and returns keyboard focus to the live monitoring status until Monitoring accepts new data.

The overlay Start/Stop handler now rechecks the native capture status at activation time, refusing duplicate requests during Starting or Reacquiring and announcing that the existing transition remains in progress.

Live-analysis Starting, Reacquiring, Error, and Stopped transitions now also invalidate overlay evidence even when the native capture bridge remains connected; Tracking clears that marker only after the capture bridge is Monitoring and announces fresh accepted evidence.

The visualization-specific Retry monitoring control now enters the same stale-evidence recovery boundary before dispatching capture start, announcing that prior overlay evidence is provisional until fresh monitoring data arrives.

Resume recovery now rechecks the user-paused marker at activation time; a stale Resume click is rejected without another native show request and returns focus to the overlay recovery control.

Hide overlay now rechecks user-hidden intent at activation time; a stale second Hide click is rejected without another native hide request and returns focus to Restore overlay.

Estimate attribution repeats the stale-capture marker as well, keeping the combined-output source label honest while its last accepted values await reacquisition.

The valid persisted visual-state restore path is directly exercised in QML regression tests, including width, height, opacity, and the resulting accessible recovery announcement.

Overlay geometry now exposes the persisted/native position and size in Settings and announces restored or clamped geometry through the shared recovery status. When native geometry is unavailable, the status explicitly keeps the control-window fallback visible.

Overlay relaunch now reports when persisted geometry was restored and keeps the control window raised and active when native restoration is unavailable or fails. This preserves a usable tray/taskbar fallback instead of leaving the user with a hidden or ambiguous control surface.

Settings now exposes a dedicated Restore overlay affordance. It follows the production relaunch path, which reuses persisted display, position, size, and opacity state, while retaining the visible control-window fallback if the native overlay cannot be restored.

The restore status now surfaces the native bridge's persisted display and opacity alongside geometry. If the bridge cannot provide those values, the UI labels the restore state unavailable and keeps the control-window fallback explicit.

Reapply saved settings now performs the native display, geometry, and opacity calls before restoring the overlay, making the persisted-state path executable from Settings rather than informational only.

Click-through unlock now shares one recovery function across the Escape shortcut and Settings button. It restores native pointer/keyboard interaction, announces the transition, and returns focus to the unlock control so the fallback remains keyboard-operable.

Overlay geometry and action groups now expose recovery-pending state through accessible names, values, descriptions, and tooltips. Missing-display, failed-reapply, explicit hide/show, and window-close paths keep the control-window fallback visible and actionable; local contract checks cover these transitions, while native monitor/compositor timing remains a qualification requirement.

Failed overlay show and visual-restore transitions now share a production fallback helper that shows, raises, and activates the control window, with an explicit tray/taskbar fallback announcement. The native overlay remains hidden until restoration succeeds, so users always have a discoverable recovery surface.

Explicit hide/unhide now tracks user intent separately from monitor-loss recovery: hiding preserves native display/geometry/size/opacity settings and reports “hidden by user,” while a successful restore clears that transient marker. Quit and relaunch reset the marker so stale visibility intent cannot leak between sessions.

On relaunch, native bridge synchronization explicitly marks an “launch fallback active” state when the overlay bridge or display list is unavailable. The control window remains the discoverable fallback until a native target returns; a successful synchronization clears the launch fallback marker.

The overlay action group now surfaces both launch fallback and explicit hidden-by-user state, so the primary Show/Restore/Reapply/Hide controls remain understandable even when the native overlay is unavailable or intentionally hidden.

Bridge availability and display-list transitions now announce action enablement changes directly: actions are disabled with the control fallback during loss, restored after native synchronization, and gated until a replacement display is explicitly chosen.

Show, Restore, and Reapply now share that display-target gate in their actual enabled state. When a saved monitor is missing, each control reports “choose a display first” and the display selector remains the recovery focus; no action claims success before a native target exists.

Activating a valid replacement display now announces that Show, Restore, and Reapply are enabled and moves focus to the retry/recovery control. Failed native selection returns focus to the display selector, preserving a keyboard-complete recovery path.

Display enumeration changes now follow the same focus contract: unplugging or losing all displays focuses the disabled selector, an unknown replacement target focuses it again, and a valid saved target returns focus to Retry after recovery. This keeps action enablement and keyboard recovery synchronized across unplug/replug transitions.

Native geometry changes (including DPI/layout updates and monitor replacement) now resynchronize the preferred size and can complete a pending recovery when the saved display target is valid. Focus returns to Retry after the geometry path succeeds; native compositor timing still requires platform qualification.

Geometry callbacks now update the preferred display, native position/size-derived preference, and an accessible geometry revision through one synchronization helper. The settings summary therefore reflects a coherent post-layout state rather than mixing values from different monitor/DPI events.

The settings synchronization path now validates native opacity alongside display, position, and size, then advances one accessible settings revision. The summary and recovery gate therefore describe a single coherent bridge snapshot across DPI/layout changes.

Direct display and size edits flow through native geometry callbacks to advance that same settings revision, while opacity edits advance it at the control boundary. The settings summary cannot silently combine a newly edited control with an older native geometry snapshot, and no edit is counted twice when the bridge echoes the change.

Opacity edits now carry an in-flight marker: the originating control edit advances the revision once, while its native bridge echo only clears the marker; unrelated native opacity changes still advance the revision. This keeps concurrent local/native updates visible without duplicate counts.

Rapid opacity edits now retain the latest pending value and match native echoes by value, not arrival order. An older/out-of-order echo cannot overwrite the slider or clear the newer edit’s pending state; the revision remains one increment per user edit.

Bridge disconnect/reconnect now clears the pending opacity marker and in-flight flag before resynchronizing native settings. Reconnect cannot apply a stale slider echo or drift the accessible settings revision; the fallback and restored-action announcements remain explicit.

The Windows native bridge now persists the display target after screen-added, screen-removed, and native screen-change callbacks complete their keep-on-screen adjustment. This keeps replacement-monitor selection durable across the QML/native boundary instead of leaving the removed display name in settings.

The macOS native screen-recovery timer now persists the clamped geometry and emits both geometry and screen-list updates after DPI/layout or monitor changes. QML can therefore refresh its atomic settings revision and focus/action gate from the same post-recovery snapshot on both native paths.

macOS direct display and size edits now explicitly schedule native persistence after clamping, matching the Windows bridge’s immediate persistence semantics and ensuring settings survive a subsequent reconnect even when no additional window signal arrives.

The overlay action group now reports the same settings revision as the detailed settings summary, alongside hidden-user and pending-recovery state. Assistive technology can verify that action availability and persisted settings describe one bridge snapshot.

Display recovery now prefers the persisted display name when the monitor list changes, falling back to the prior session index only when necessary. This keeps a reconnected display target stable across enumeration order changes and otherwise leaves the explicit display-selection fallback active.

The QML surface now synchronizes the preferred display identity from the native bridge's persisted state and shows targeted missing-display guidance when that saved monitor is not enumerated. Users can reconnect it or select a replacement without losing the control-window recovery path.

Missing-display recovery now includes an explicit Forget saved display action. It clears the stale preferred identity and session index, announces that replacement selection is ready, and prevents an unavailable monitor from being repeatedly reselected.

Replacement display selection now goes through a shared persistence helper that invokes the native bridge's move-to-screen operation before updating QML fallback state. The selected display is therefore persisted by the platform bridge, not only remembered in the current QML session.

On relaunch, the control window hydrates its preferred display identity and index from the native bridge after attachment. If the persisted identity is stale or absent from the current monitor list, selection-required state is raised immediately so the user can reconnect, forget, or replace the target.

Relaunch hydration now validates native geometry and opacity as well. Valid persisted geometry is reapplied through the bridge and announced; missing or invalid geometry explicitly keeps the control-window fallback active rather than presenting an unpositioned overlay.

Invalid persisted opacity now falls back to a safe 90% value before any overlay show operation and announces the correction, preventing malformed native settings from producing an unusable or invisible overlay.

Every overlay show, retry, and restore path now reapplies validated visual state immediately before the native show call, keeping geometry and opacity recovery consistent after relaunch, display replacement, and transient native failures.

Overlay hide now always restores and activates the control window, including the overlay's own close affordance. A dedicated Quit Beatglass action hides the native overlay before exiting, preserving persisted overlay settings and making the shutdown lifecycle explicit to assistive status consumers.
Overlay availability recovery now synchronizes the action surface as a single transition: losing the native bridge marks recovery pending, while a reconnect refreshes the settings revision and distinguishes “no display available” from “display selection required.” Accessible focus and announcements direct the user to the display selector when actions cannot yet be used; once the selected display is valid, the recovery marker clears and Show, Restore, and Reapply report availability together.

Click-through state now follows the same recovery boundary: a disconnected bridge clears the stale toggle, a reconnect hydrates the native mode, and Unlock remains unavailable unless the live bridge is both available and click-through. The routing summary includes the settings revision so assistive technology can correlate interaction mode and visual settings.

Click-through transitions participate in the settings revision as well. The native mode announcement and accessible routing summary therefore advance together whenever pointer interaction changes, including a reconnect-driven mode hydration.

If the native platform rejects a requested click-through transition, the toggle is restored to the actual native state and an actionable recovery announcement keeps the control window discoverable instead of claiming that input mode changed.

Unlock follows the same failure contract: the control window only announces restored interaction after the native bridge accepts the transition; a rejected unlock keeps fallback controls available and reports the failure.

Hide also restores the control window when the native bridge has already disappeared, preserving the user-hidden intent while making the fallback actionable.

Persisted display restore failures now announce that the saved display is unavailable and direct keyboard users to choose another display while retaining the control-window fallback.

Restore failures now also return keyboard focus to the display selector after reactivating the control window, covering both an unavailable persisted display and invalid persisted visual state.

The unavailable-bridge restore path now follows the same focus rule, so every persisted-restore failure leaves keyboard users at the display recovery control.

Invalid persisted visual state now announces the exact recovery action before returning focus to display selection.

Native show failures now restore focus to display recovery as well, keeping the fallback actionable when the platform refuses to present the overlay window.

Unavailable-bridge and invalid-visual-state show fallbacks now restore the same display-selector focus, so every show failure path is keyboard actionable.

Display replacement recovery and explicit display selection failures now return focus to the display selector too, covering missing saved displays, native move failures, and visual-state reapply failures.
After hiding the overlay, focus returns to the Hide control in the restored control window so keyboard users can continue from a stable action surface.

Reopen selected playback now rechecks source identity at click time, preventing stale replacement requests after selection and playback converge.

Seek-to-frame now rechecks the live playback state at click time and announces when a stale seek action is invalidated by playback stopping.

The five-second seek controls now perform the same click-time state admission, preventing stale bridge calls when playback stops between rendering and activation.

Pause/resume now rechecks the live playback state at activation time, preventing stale bridge calls and announcing when a concurrent transition invalidates the control.

Stop playback now rechecks the live state before clearing the source, making repeated or stale stop activations harmless and explicitly announced.

After a real stop admission, the playback surface announces that the source is ready to reopen or replace, giving keyboard and screen-reader users a clear next state.

Successful seek-to-frame admission now announces the requested frame, making the resulting playback transition observable without relying on visual position updates alone.

Native playback start now announces the selected-output admission request, making the transition observable while the bridge moves to its running state.

Refreshing playback outputs now returns focus to the output-device selector, making the refreshed device list immediately actionable from the keyboard.

Retry native playback now rechecks the live bridge error at click time and announces when a stale retry action has been invalidated by a state transition.

The shared fallback-control helper now restores display-selector focus after activation, ensuring every caller that presents the control window provides the same keyboard recovery entry point.
Active analysis failure reasons, recovery status, and persisted queue errors now expose their live text as accessible status values, so assistive technology receives the same diagnostics visible in the recovery surface.
The display selector now includes recovery-pending state in its accessible value and description, so assistive technology receives the same recovery context as the surrounding display status.

The Hide action now reports whether it is available or already hidden, and explains the control-window fallback when native overlay availability is lost.

Quit now reports that it is available and will retain persisted overlay settings, making the shutdown contract explicit to assistive technology.

Show now includes recovery-pending context in its accessible value while remaining actionable, matching the surrounding overlay action group state.

Restore now exposes the same recovery-pending context, including when restoring an overlay hidden by the user.

Reapply saved settings now reports recovery-pending context in its accessible value, completing the consistency across Show, Restore, and Reapply actions.

Unlock now reports the same recovery-pending context when click-through interaction is active, keeping every overlay action’s accessible state coherent.

The Click through checkbox now exposes recovery-pending context directly, not only through its parent routing group.

The opacity slider now carries the same recovery-pending state in its value and description, keeping direct visual controls aligned with the overlay action surface.

The interaction-mode status now carries recovery-pending context directly, so click-through and interactive states remain understandable while display recovery is in progress.

The routing-controls group description now announces recovery-pending state too, giving screen readers context before they enter its individual controls.

Display-change recovery now checks the native show result. If the platform refuses to present the overlay after the saved display is restored, recovery remains pending and the control-window fallback gives an actionable retry/display-selection path.

Display-change recovery now preserves explicit hidden-by-user intent: the saved display and visual state are repaired without auto-showing the overlay, and the user is directed to Restore overlay when ready.

The current-display status now exposes hidden-by-user intent directly, so assistive technology can distinguish a recovered display from an intentionally hidden overlay.

The display-and-size controls group now repeats both hidden-by-user and recovery-pending context before users enter its individual controls.

Geometry status now repeats hidden-by-user intent alongside its revision and recovery state, so persisted position/size announcements do not imply that the overlay is currently visible.

Successful display selection now synchronizes the accepted native display/geometry/opacity snapshot before publishing the preferred display, keeping settings revision and accessibility summaries atomic.

Failed native display movement or visual-state reapplication now raises display-selection-required as well as recovery-pending, preventing Show/Restore/Reapply from claiming a usable destination until the user chooses a replacement.

Monitor replacement recovery applies the same gate when the display moves but persisted visual state fails, requiring an explicit display selection before actions become available again.

Programmatic or stale display indices now enter the same selection-required gate and return focus to the display selector before any overlay action can proceed.

Stale display selections now announce that replacement is required, so the selection gate is both actionable and discoverable to assistive technology.

If presentation fails after display recovery, the app now raises the same selection-required gate before retrying, preventing actions from claiming the recovered destination is usable.

Stale display selections now clear the preferred index and name as part of that gate, preventing later recovery attempts from reusing invalid monitor identity.
The persisted recovery guidance itself is now exposed as a live status value, carrying unavailable, retryable, or clear state to assistive technology as storage transitions and retryable-run counts change.

Storage transition, persisted retry outcome, and stale-result diagnostics now expose their current text as accessible status values, keeping database reopen and retry decisions synchronized for keyboard and screen-reader users.

Batch result, persisted recovery, retry rejection, admission, and frame-progress diagnostics now expose their live text as status values, completing the accessible readout for persisted analysis lifecycle states.

Cache availability, cached results, worker results, and queue outcomes now expose live accessible status values, completing the result and admission readout across persisted and active analysis paths.

Output endpoint discovery, selected endpoint state, and capture-permission guidance now use live status semantics, so device recovery and permission instructions are announced consistently.

The native overlay-unavailable fallback now uses the same live status semantics as geometry, persisted restore, settings, and historical recovery diagnostics, making display loss consistently discoverable.

Visualization interaction, selection reset, analysis handoff, and playback-epoch feedback now expose live status semantics for capture recovery and generation alignment.

Overlay interaction mode, opacity, and display destination now expose live status values, keeping pointer routing and display recovery state consistent for assistive technology.

DJ monitoring status and BPM/key/chord estimates now use live status semantics, completing the overlay telemetry readout.

The overlay fallback status now includes the concrete Retry overlay recovery action, so assistive technology receives both the fallback availability and the next step.

For a missing saved display, Forget saved display is keyboard reachable, announces the replacement-display next step, and restores focus to the display selector after clearing stale destination state.

Retry overlay is now explicitly included in the keyboard tab order while display recovery is pending, so users can reach the actionable retry control without pointer input.

The overlay display selector is now explicitly keyboard reachable as well, ensuring recovery can proceed from destination selection through retry without pointer input.

Display recovery now commits the native bridge's canonical display identity after geometry and visual-state synchronization succeeds, preventing a stale picker label from being retained across relaunch while keeping the control-window fallback transactional on failure.

Reapply now rejects a persisted display index that is outside the currently enumerated displays before calling the native bridge, returning focus to destination recovery instead of attempting an invalid restore.

When no destination display is enumerated, display-selection recovery now explicitly marks overlay evidence stale so the live status matches the unavailable native destination.

The output endpoint empty state now uses live status semantics alongside endpoint discovery and recovery guidance, so missing-device remediation is announced consistently.

The output-device selector is explicitly keyboard reachable, keeping monitoring setup usable from the same keyboard recovery path as endpoint refresh and status guidance.

Playback sample-rate and channel controls are explicitly keyboard reachable, keeping native output setup usable without pointer input.

Start native playback now requires a selected output device, preventing an unusable admission attempt and keeping the recovery path aligned with the actual playback bridge state.

The overlay display empty state now uses live status semantics alongside saved-display recovery, making missing-output remediation discoverable before destination selection.

Appearance guidance, current theme/motion state, and settings persistence safety now expose live status semantics, keeping preference changes and the user-audio safety guarantee available to assistive technology.

Metadata status, metadata recovery guidance, visualization data, and visualization capture state now use live status semantics, aligning source, analysis, and rendering diagnostics for assistive technology.

Live tempo, harmony, overlay capture, and monitor capture states now use status semantics, keeping the primary overlay and monitoring readouts synchronized for assistive technology.

Analysis freshness, selected-source workflow, and frame progress now expose live status values, aligning source selection and active analysis lifecycle feedback.

Native playback state, source, position, and output-running state now expose live status values, making playback and analysis source identity consistent for assistive technology.

Native playback errors, seek limitations, source-selection mismatch, and unavailable-control guidance now use live status semantics, keeping playback recovery actionable.
