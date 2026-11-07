# Design system: Beatglass v1

## Intent and scope

A colorful, legible desktop workspace for listening and inspecting evidence. The user's six requested themes share geometry and meaning. Color is expressive but never the only status signal. Use the theme tokens in `style.css` verbatim as the canonical numerical values; each `body[data-theme]` block overrides the base tokens. Translate pixels to Qt device-independent pixels and use native OS controls and text scaling where needed. This document specifies the prototype, not a claim that these controls or data exist in the app.

## Foundations

- Base text: Segoe UI with Arial/sans-serif fallback, 14 px; foreground `--ink`, background `--bg`, secondary text `--muted`. H1 30 px, bold, −1 px tracking; H2 19 px bold; H3 14 px bold. Eyebrows 10 px, bold, 1.4 px tracking, uppercase and `--violet`. Details use 12 px. User text scaling must override these reference sizes without clipping.
- Layout grid: 210 px navigation rail; main content 42 px horizontal and 36 px top inset; 12, 20, 24 and 32 px spacing rhythm. At 900 px, rail 165 px and content 24 px; stack two-column panels. At 650 px, rail 58 px, content 16 px, preserve table columns in a horizontal scroller. Native minimum window and platform behavior need validation.
- Surfaces: panel `--panel`, inset/control `--panel2`, hairline `--line`, 8 px panel and 6 px control corners. Panel shadow `0 8px 24px --glow`; primary button `0 4px 18px --glow`. Heading text shadow `0 0 24px --glow`; brand `0 0 18px --glow`. Shadows are decoration, never required to discern a boundary. In high contrast retain visible borders even if effects are disabled.
- Spectral colors: base `--pink #f49acb`, `--violet #c4a8ff`, `--cyan #86def2`, `--gold #ffd080`; light theme overrides them with darker counterparts. Tempo uses accent, key cyan, tonal evidence pink; alternating chord labels use pink, cyan and gold. Labels explain the meaning. Links are bold, underlined with 3 px offset. Explanatory hints are italic, 12 px. Limit decorative emphasis on long prose.
- Six themes: Light uses pale green-white surfaces and deep teal text; Dark uses blue-black with mint; Contrast uses black, white and bright mint with stronger edges; Purple uses plum surfaces and violet primary; Blue uses deep navy and cyan; Yellow uses brown-black and gold. `--accentink` is used on filled primary controls. Theme selection must be explicit and persist only after actual implementation.

## Atoms

**Button:** 1 px `--line` border, 6 px radius, `--panel2` fill, `--ink` label, 10 px vertical / 14 px horizontal padding. Primary uses `--accent` fill and border, `--accentink` bold text and glow. Hover changes border to accent; keyboard focus is a 3 px accent outline with 2 px offset. Disabled controls need a text/semantic disabled state and sufficient distinction; implement and test natively. Destructive actions must name the effect, not depend on red alone.

**Input/select:** same 1 px border, 6 px radius and inset surface, with visible associated label and focus outline. Search has a leading search symbol, 9×12 px padding and a 200 px reference input. Select fills its allocated column. Checkboxes and sliders use native semantics; sliders expose label, numeric value and keyboard adjustment.

**Status tag:** inline, 11 px bold, 1 px current-color outline, 4 px radius, 4×7 px padding. Good uses accent; informational uses blue; attention uses warning. The text itself says Ready, Analyzing, Source changed, Provisional or Unavailable. Never imply a calibrated confidence score.

**Indicator / meter:** status dot is 8 px round, accent; accompany with a text label. Activity meter is an 8 px track in `--line`, accent fill, and explicit Signal detected / No signal text. It represents activity, not a calibrated level.

**Type and links:** primary content uses `--ink`; supporting copy uses `--muted` with 1.5 line height. Use bold for labels and result hierarchy; underlining identifies actual links. Do not make static text look interactive in the native implementation.

## Components

**Navigation rail:** rail surface and right hairline; brand at top, 16 px outer padding; navigation items 12 px inset, 6 px radius, 14 px labels with 12 px icon gap. Active item gets selected fill, accent outline and bold text. Bottom workspace message remains subordinate. Provide accessible names when labels collapse; keyboard navigation must follow platform conventions.

**Page header:** eyebrow, title, explanatory line on left; actions on right with 12 px gap. Wrap actions and stack header at narrow widths. Keep source and active context visible.

**Summary strip / metrics:** one bordered panel, equal columns divided by hairlines, 20×22 px inset. Value is 26–28 px bold, label 10 px uppercase; explanation 12 px. Metrics are independent of color coding. Use em dash for missing evidence, never stale values.

**Track table:** header 38 px, rows at least 62 px, 20 px horizontal inset. Columns track/artist/path, duration, BPM, key and labeled status. Track name bold, secondary metadata 12 px; selected row uses selected fill and a visible edge. Actual table must support sorting, selection, keyboard navigation, long names and accessible column headings.

**Timeline:** contiguous equal-width illustrative cells in a bordered, 6 px clipped strip; 16×12 px cell padding, hairline separators, 18 px bold chord and 11 px timestamp. Current cell has selected fill and 3 px accent top rule. Real segments must reflect time, allow keyboard seeking, expose timestamps and current playhead, and handle unknown/ambiguous windows. The mock's uniform widths are illustrative, not an analysis algorithm.

**Player:** circular accent play button, elapsed and total time, 6 px progress track and volume control; all actions need accessible labels, seek semantics and keyboard controls. Playback is optional for analysis.

**Banner:** 15×18 px inset, 1 px border, 6 px radius, title plus explanation and action. Live/attention borders reinforce the written state. Recovery copy identifies source and next action, and stale results disappear on selection changes, cancellation or discontinuity.

**Queue row:** 20 px vertical padding, source name and filename, labeled state or progress bar, action. Progress is numeric; cancel and retry are explicit and scoped to the displayed job. Batch controls must not delete source files.

**Overlay:** 520 px reference width (420 px in the contextual scene), translucent near-black `#111b21ed`, 1 px `#718986` border, 10 px radius, 20×24 px padding and `0 12px 40px #0006` shadow. Top line identifies live source and provisional status; three equal columns show 38 px bold BPM, key and current chord, with 10 px labels; recent chord history follows a hairline. Avoid assuming its background will be dark: retain sufficient opacity and legibility over arbitrary software. Provide compact/small and resized native variants, beat pulse with reduced-motion fallback, placement and monitor selection. The control window must offer Unlock / edit and Hide even when click-through is active. Restore controls from taskbar or a platform-supported tray entry; do not rely on a tray existing on every platform. Persist position only after validating monitor geometry and provide an off-screen recovery path.

## Behavioral boundaries

Source attribution accompanies every estimate. Source changes, cancellation and capture discontinuities clear obsolete evidence and reacquire. Key/chord windows and scores are provisional and uncalibrated; simultaneous decks are a combined mix. No tag writing or source-file deletion is implied. The presentation is aspirational: cache reuse remains disabled until identity verification, and codec/platform availability must follow actual support. The mock does not define Qt contracts or replace native platform validation.

**Track-level key versus tonal windows:** An analysis job completing does not validate a track-level key. Until a validated track-level estimator exists, the Library `TRACK KEY` cell displays an em dash with an accessible “unknown” description, and the detail metric says “Unknown · No validated track-level key.” A time-attributed rolling-window estimate may appear separately only with its timestamp and a visible “provisional” label; alternatives and uncalibrated scores must not be presented as track-level facts. If a validated track-level estimator becomes available, its output must be identified distinctly from window evidence. All sample values in these renders are illustrative.

### Overlay activation and return

When the user shows the overlay, keep monitoring active and hide the main window. On Windows and Linux, remove the main window from the taskbar and provide a persistent tray icon; on macOS, hide the main app/window and provide a menu-bar icon. The icon opens a menu with Show Beatglass, Unlock / edit overlay, Hide overlay and Quit. Reopening the application restores the existing instance and its main window. The overlay itself needs no window-restore button. If a tray or menu-bar icon cannot be provided, do not hide the only recovery path: retain an accessible main window or taskbar entry and explain the fallback. Do not interrupt other apps or playback. Show the transition clearly on first use and preserve a way to recover an off-screen overlay. Confirm platform conventions, single-instance behavior, accessibility and compositor restrictions in native testing; these mockups do not demonstrate them.

## Recovery placement (updated)

Output device and audio permission controls are Settings sections. macOS instructions must match the actual capture API and OS version; the illustrative copy is not verified native guidance. When a user interacts with an unavailable library file, show a contextual alert with locate, remove-entry and cancel actions and explicitly preserve the original audio. When analysis stops, show an in-app toast and an OS notification when available and permitted; the toast and job state remain the fallback if notifications are disabled. No standalone recovery dashboard is required.

Theme cards are selectable controls and preview their own theme palette. Expose selected state and keyboard access. Click-through and Reduce motion use clearly labeled, keyboard-accessible switches with visible focus and non-color state cues. Real display selection enumerates connected monitors and initially selects the display containing the control window; recover safely on monitor removal.

## Audio visualization and playback (design intent)

Reserve a labeled audio view above the Library player and a compact visualization strip in the overlay. The user chooses Waveform or FFT. Use colorful, theme-aware gradients, but draw only data derived from the actual selected playback stream or monitored output. Waveform min/max buckets scroll in source time; FFT uses frequency on the horizontal axis and labeled magnitude semantics. Pause or hold the visual when playback stops, clear it on seek/source change/discontinuity, and show waiting/unavailable instead of invented motion. Respect Reduce motion with a stable or lower-update-rate view. Player controls need play/pause, seek, time, volume and accessible keyboard semantics; absent output devices must not block analysis. The static bars in this prototype are explicitly illustrative and are not an audio implementation.

## Optional DJ track metadata

The default DJ overlay analyzes the selected system output's combined mix; it does not know which tracks or decks produced it. Show “Track info unavailable” when metadata is not supplied, and do not label output-derived tempo, key, chords, waveform or FFT as belonging to a particular track or deck. The DJ application shown underneath the overlay in the contextual mock is illustrative: its deck titles and waveforms belong to that application, not information Beatglass has read.

MIDI timing or controls do not establish a universal live title/artist feed. A future, explicitly configured software-specific integration may expose metadata through a documented API, plugin, or a supported device-specific MIDI protocol. Do not present such an integration as available before it is implemented and tested. If added, show its connection/source and deck, identify whether data is loaded, playing or otherwise known, and clear stale metadata immediately on disconnect or deck change. A loaded deck is not necessarily audible; overlapping decks cannot be assigned the combined-output analysis without separate evidence. Provide a disconnected/unavailable state and preserve the capture-only overlay. Verify permissions, licensing, protocol and behavior for each supported application and OS before making support claims.
