# Issue #76 visual implementation audit

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

DJ monitoring status, estimates, evidence quality, and monitoring guidance now expose contextual tooltips, making live-state interpretation and recovery guidance consistent across the visualization and overlay surfaces.

The DJ page heading and live-output monitoring summary now expose explicit heading semantics and a concise state tooltip, making the monitoring context discoverable before users reach the action controls.
