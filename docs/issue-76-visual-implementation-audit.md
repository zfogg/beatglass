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
