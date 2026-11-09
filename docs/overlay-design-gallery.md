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
| [Quick actions](overlay-quick-actions-card.svg) | high-frequency controls and shortcuts |
| [Diagnostics](overlay-diagnostics-card.svg) | overlay, capture, and permissions health |
| [Help](overlay-help-card.svg) | shortcuts, troubleshooting, and about |
| [Performance](overlay-latency-card.svg) | latency, sample rate, and dropped frames |
| [Notifications](overlay-notifications-card.svg) | restore, output, and pause activity |
| [Calibration](overlay-calibration-card.svg) | live preview for position, size, opacity |
| [Privacy](overlay-privacy-card.svg) | local analysis and safe file behavior |
| [Startup](overlay-startup-card.svg) | saved setup and readiness checklist |
| [Minimized](overlay-minimized-card.svg) | quiet monitoring with retained layout |
| [Reconnecting](overlay-reconnecting-card.svg) | retrying a lost selected output |
| [Listening](overlay-listening-card.svg) | healthy live output analysis |
| [Pause confirmation](overlay-pause-confirmation-card.svg) | explicit capture pause decision |
| [Resume](overlay-resume-card.svg) | ready-to-continue monitoring workflow |
| [Status bar](overlay-status-bar-card.svg) | compact live monitoring indicators |
| [Contrast](overlay-contrast-card.svg) | readable themes, labels, and state colors |
| [Level](overlay-level-card.svg) | live audio level and headroom guidance |
| [Now playing](overlay-track-card.svg) | track context alongside live analysis |
| [Up next](overlay-queue-card.svg) | current and queued track context |
| [Session summary](overlay-session-summary-card.svg) | session duration, tracks, health |
| [History](overlay-history-card.svg) | local recent listening record |
| [Pinned views](overlay-favorite-card.svg) | favorite layouts and outputs |
| [Tips](overlay-tips-card.svg) | compact guidance for personal workflows |
| [Command palette](overlay-command-palette-card.svg) | searchable keyboard-first actions |
| [Update](overlay-update-card.svg) | update review and defer decision |
| [Reset](overlay-reset-card.svg) | reversible settings reset decision |
| [Import](overlay-import-card.svg) | saved view import and scope explanation |
| [Export](overlay-export-card.svg) | saved view export and local scope |
| [Rename](overlay-rename-card.svg) | favorite view naming and scope |
| [Remove pin](overlay-delete-card.svg) | reversible favorite shortcut removal |
| [Permission](overlay-permission-card.svg) | transparent system audio access request |
| [Permission denied](overlay-permission-denied-card.svg) | actionable access recovery state |
| [Permission pending](overlay-permission-pending-card.svg) | in-progress access confirmation state |
| [Permission granted](overlay-permission-granted-card.svg) | successful access and next step |
| [Access changed](overlay-access-revoked-card.svg) | permission loss and recovery action |
| [Access restored](overlay-access-restored-card.svg) | successful permission recovery state |
| [Platform support](overlay-platform-card.svg) | per-platform qualification status |
| [Release notes](overlay-release-notes-card.svg) | concise update highlights |
| [Feedback](overlay-feedback-card.svg) | lightweight in-context feedback prompt |
| [Feedback sent](overlay-feedback-submitted-card.svg) | confirmation and return-to-work state |
| [Feedback skipped](overlay-feedback-dismissed-card.svg) | non-blocking feedback dismissal state |
| [Feedback privacy](overlay-feedback-privacy-card.svg) | explicit data-sharing boundary |
| [Feedback offline](overlay-feedback-offline-card.svg) | non-blocking offline feedback state |
| [Feedback retry](overlay-feedback-retry-card.svg) | in-progress feedback submission state |
| [Feedback failed](overlay-feedback-failed-card.svg) | recoverable feedback failure state |
| [Library search](overlay-library-search-card.svg) | local track search with overlay continuity |
| [Library empty](overlay-library-empty-card.svg) | no-results guidance and add-folder action |
| [Library loading](overlay-library-loading-card.svg) | indexing progress with live overlay continuity |
| [Library ready](overlay-library-complete-card.svg) | completed indexing and continuation actions |
| [Add folder](overlay-library-add-folder-card.svg) | local library folder selection and scope |
| [Scan options](overlay-library-scan-options-card.svg) | full or incremental local indexing |
| [Scan paused](overlay-library-scan-paused-card.svg) | resumable indexing with live monitoring |
| [Scan canceled](overlay-library-scan-canceled-card.svg) | safe canceled indexing and resume action |
| [Scan failed](overlay-library-scan-failed-card.svg) | recoverable indexing failure with diagnostics |

The gallery is intentionally state-first: every card pairs status, retained
context, and a next action. See [overlay recovery workflow](overlay-recovery-workflow.md)
for the interaction sequence and screenshot qualification checklist.

The qualification matrix below maps the committed renderings to the native
desktop evidence still required on each platform:

![Overlay qualification matrix](overlay-qualification-matrix.svg)
