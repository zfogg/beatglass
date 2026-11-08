# Issue #76 visual implementation audit

Overlay-unavailable diagnostics are cleared after successful retry or explicit discard: stale run identity and rejection text disappear before the restored overlay state is announced. Failed recovery keeps diagnostics visible and actionable, preserving a truthful fallback path.

After fresh source and display selection, a successful recovery clears the persisted retry rejection and stale identity message before presenting the new accepted state. If recovery fails, the rejection remains visible and actionable; the cleanup boundary is covered portably, while native recovery timing remains open.

When overlay recovery is unavailable, persisted retry rejection remains announced and keyboard-focusable even with reduced motion enabled. The message names the blocked identity/decoder/source condition and points to fresh source and display selection; restoring a compatible source/display clears the stale rejection before retry.

Persisted retry rejection is visible alongside overlay-unavailable/retry states: stale identity, incompatible decoder snapshot, or missing source metadata is reported as a blocked retry, not a successful overlay recovery. The rejection message remains focusable and the user can choose a fresh source/display target; native timing remains open.

Overlay Retry is readiness-gated by an available selected display. The unavailable/display-loss state keeps Retry disabled while no target exists, preserves reduced-motion behavior, and re-enables the action only after display restoration; the accessible value distinguishes “available” from “waiting for a display.”

Overlay Retry is target-specific: it uses the selected display, announces the retry, and returns focus to the display selector after restoration. If no target is available, the unavailable state remains actionable without claiming recovery; native display-loss timing remains open.

Display-loss evidence must show the overlay disappearing, the control window remaining available, an accessible display-unavailable announcement, and Retry succeeding after restoration. The QML fallback restores controls automatically; native monitor loss/recovery, click-through, and multi-display timing still require retained evidence.

Overlay design states are covered by explicit Show, Unlock, Hide, display-change, unavailable-display, empty-display, and Retry controls. Each state keeps recovery guidance accessible and routes display changes through the native overlay bridge; native multi-monitor, click-through, and display-loss evidence remains required.

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

Endpoint bridge transitions now actively clear source-bound visualization selection, distinguish retry failure from no-device reacquisition, and restore focus to the endpoint selector. A replacement accepted snapshot is therefore required after endpoint replacement rather than reusing stale waveform/FFT state.

Settings endpoint, DJ monitoring, and overlay availability now share the same capture-recovery reset path and no-device/reacquisition language. A failed monitor or unavailable overlay clears stale visualization selection and returns focus to the recovery target.

Overlay hide, unlock, and display changes now use that same helper. Their announcements and focus restoration remain immediate even with reduced motion enabled, while any output reacquisition still requires a fresh accepted snapshot.

Overlay show and screen-list transitions use the same fallback contract: a missing display restores control-window guidance, while a restored display announces that a target must be chosen before overlay monitoring resumes.

Retry overlay is now disabled until a display target exists. Once a display returns, the explicit retry action announces its attempt and keeps the control window available until the native overlay actually restores.

After display restoration, Retry remains disabled until the user explicitly chooses a target. Selection clears that gate, restores focus to Retry, and announces readiness; reduced motion does not alter this keyboard/focus contract.

When the overlay is unavailable, the recovery panel may show the persisted admission/rejection bridge result only for failed or cancelled analysis, explicitly labeling it historical rather than current visualization data. This prevents retry diagnostics from making stale-result claims.
