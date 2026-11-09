# Overlay qualification evidence guide

Use the committed renderings as the visual baseline when collecting native
desktop evidence for issue #76. Capture each state on Linux, macOS, and Windows
with the control window and overlay visible at the platform's standard desktop
scale.

| Capture | Baseline rendering | Required observation |
| --- | --- | --- |
| Ready | [`overlay-ready-card.svg`](overlay-ready-card.svg) | overlay visible; settings applied |
| Recovery | [`overlay-settings-card.svg`](overlay-settings-card.svg) | retained settings and Review recovery |
| Disconnected | [`overlay-disconnected-card.svg`](overlay-disconnected-card.svg) | control window visible; Retry overlay |
| Interaction | [`overlay-interaction-card.svg`](overlay-interaction-card.svg) | click-through and Unlock overlay |
| Capture | [`overlay-capture-card.svg`](overlay-capture-card.svg) | source/status and reacquisition |

Record platform, build identity, display topology, source, state transition,
screen dimensions, screenshot paths, and limitations beside each native capture.
Headless contract tests and these SVGs organize evidence but cannot mark a
platform `READY` without the real desktop/audio observation.
