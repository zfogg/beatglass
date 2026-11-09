# Overlay design evidence gallery

This gallery is the review index for the issue #76 overlay design system. Each
rendering is committed so reviewers can inspect the state vocabulary without a
running native desktop session; platform qualification still requires native
screenshots.

| Evidence | State or component |
| --- | --- |
| [Overview](overlay-design-system-overview.svg) | status → settings → actions → announcements |
| [Design tokens](overlay-design-tokens.svg) | state colors and action vocabulary |
| [Ready card](overlay-ready-card.svg) | restored overlay |
| [Settings card](overlay-settings-card.svg) | connected recovery |
| [Disconnected card](overlay-disconnected-card.svg) | display unavailable |
| [Tray fallback](overlay-tray-fallback.svg) | monitoring retained |
| [Recovery timeline](overlay-recovery-timeline.svg) | loss → restore |
| [Recovery workflow](overlay-recovery-workflow.svg) | fallback decision flow |
| [Window chrome](overlay-window-chrome.svg) | compact live overlay |
| [Visualization](overlay-visualization-card.svg) | waveform empty/ready |
| [FFT](overlay-fft-card.svg) | frequency-domain readout |
| [Capture](overlay-capture-card.svg) | output reacquisition |
| [Display](overlay-display-card.svg) | selected/unavailable display |
| [Size](overlay-size-card.svg) | compact/medium/large presets |
| [Opacity](overlay-opacity-card.svg) | bounded visual control |
| [Interaction](overlay-interaction-card.svg) | click-through/unlock |
| [Shortcuts](overlay-shortcuts-card.svg) | keyboard parity |
| [Announcements](overlay-announcement-card.svg) | accessible state copy |
| [Error](overlay-error-card.svg) | actionable failure state |
| [Layout](overlay-layout-card.svg) | spacing and hierarchy |
| [Reduced motion](overlay-reduced-motion-card.svg) | static visualization preference |
| [Theme](overlay-theme-card.svg) | contrast-safe visual preference |
| [Focus](overlay-focus-card.svg) | keyboard focus visibility |
| [Live status](overlay-live-status-card.svg) | current output and signal health |
| [Control panel](overlay-control-panel-card.svg) | opacity, placement, and interaction |
| [Compact mode](overlay-compact-mode-card.svg) | size presets with live preview |
| [First run](overlay-first-run-card.svg) | output, size, and start workflow |
| [Paused state](overlay-paused-state-card.svg) | safe pause and resume behavior |
| [No signal](overlay-no-signal-card.svg) | waiting state before playback |
| [Output picker](overlay-output-picker-card.svg) | selected, available, unavailable outputs |

The gallery is intentionally state-first: every card pairs status, retained
context, and a next action. See [overlay recovery workflow](overlay-recovery-workflow.md)
for the interaction sequence and screenshot qualification checklist.

The qualification matrix below maps the committed renderings to the native
desktop evidence still required on each platform:

![Overlay qualification matrix](overlay-qualification-matrix.svg)
