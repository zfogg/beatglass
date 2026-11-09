# Overlay recovery design system

The recovery workflow uses a small, state-first vocabulary so the control window
remains understandable during display and native-bridge failures.

| Token | Value | Use |
| --- | --- | --- |
| `surface.canvas` | `#111827` | dark control-window canvas |
| `state.ready` | `#4ade80` | overlay is restored and settings applied |
| `state.recovery` | `#fbbf24` | persisted settings need review |
| `state.disconnected` | `#fb7185` | display or bridge must reconnect |
| `action.primary` | `Review recovery` | focus the persisted settings summary |
| `action.restore` | `Restore overlay` | apply retained settings and show overlay |
| `action.retry` | `Retry overlay` | retry after a display reconnects |

The status card is always paired with one next action. Status copy is concise,
while `Accessible.name`, `Accessible.value`, and `Accessible.description` carry
the state, revision, and recovery instruction for assistive technology.

![Overlay recovery design tokens](overlay-design-tokens.svg)

The settings card keeps the state, retained values, and next action in one scan:

![Overlay settings card rendering](overlay-settings-card.svg)

When no display is available, the same card keeps the recovery action stable and
changes only the next-step copy:

![Overlay disconnected card rendering](overlay-disconnected-card.svg)

The ready state keeps the same hierarchy while making the successful restore
visually quiet:

![Overlay ready card rendering](overlay-ready-card.svg)

Interaction mode uses the same status-card hierarchy so click-through state is
visible without competing with recovery actions:

![Overlay interaction mode rendering](overlay-interaction-card.svg)

Opacity uses an explicit value and a bounded slider so the visual effect is
understandable without relying on color alone:

![Overlay opacity control rendering](overlay-opacity-card.svg)

Size uses named presets with dimensions so the choice remains legible at a glance:

![Overlay size control rendering](overlay-size-card.svg)

Display selection uses the same clear selected/unavailable treatment:

![Overlay display control rendering](overlay-display-card.svg)

The tray/taskbar fallback keeps monitoring available while native overlay actions
are unavailable:

![Overlay tray fallback rendering](overlay-tray-fallback.svg)

Announcements mirror the visible state and next action so visual and assistive
feedback stay aligned:

![Overlay announcement rendering](overlay-announcement-card.svg)

Keyboard shortcuts use the same action labels as the visible controls:

![Overlay keyboard shortcut rendering](overlay-shortcuts-card.svg)

The compact overlay chrome keeps the live readout calm while exposing status and
the current visualization mode:

![Overlay window chrome rendering](overlay-window-chrome.svg)

Visualization uses a quiet empty/ready treatment with the mode and capture state
always visible:

![Overlay visualization rendering](overlay-visualization-card.svg)
