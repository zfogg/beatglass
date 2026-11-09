# Overlay recovery workflow

The control window keeps one predictable recovery path visible whenever the native
overlay is unavailable or its persisted settings need attention.

![Overlay recovery workflow rendering](overlay-recovery-workflow.svg)

```text
Ready ── display/bridge loss ──> Fallback visible + Review recovery
  ▲                                  │
  │                                  ├─ connected: review settings → Restore overlay
  │                                  └─ disconnected: reconnect display → Retry overlay
  └────────────── successful restore ┘
```

The fallback status is intentionally a status card, not a modal error. It keeps
the live monitoring controls available, states whether recovery is pending, and
offers a keyboard-focusable `Review recovery` action that moves focus to the
persisted display/size/opacity summary. The summary and action announce the
settings revision and the next recovery step through the accessibility tree.

Each explicit `Retry overlay` request is counted in the control window and
announced as a numbered attempt. The count resets when a native bridge or
compatible display becomes available, so retry feedback stays bounded and
does not imply that capture itself was restarted.

Recovery permits three attempts before the retry action enters a safe-mode
state. Reconnect a display or change recovery settings to leave that state.
The control window also exposes `Clear safe mode` so a user can deliberately
start another bounded recovery cycle after checking the display connection.
While safe mode is active, a status message remains visible and explicitly
states that monitoring continues without the native overlay.
Clearing safe mode returns focus to the overlay recovery page so the next
retry is visible and keyboard-ready immediately.
The same action is available from the keyboard with `Ctrl+Alt+Shift+C`.
The safe-mode card repeats that shortcut next to the status so recovery remains
discoverable without opening a separate help view.
During an active retry cycle, the card also shows bounded progress such as
`Recovery attempts: 1/3` so the next action and remaining budget are clear.
Users can choose `Stop recovery` before the limit to pause retries while
leaving monitoring available; starting a new retry cycle remains explicit.

## Screenshot qualification

When desktop rendering is available, capture these states at the standard desktop
viewport and attach the images to the UI qualification issue:

1. Ready overlay with the settings summary.
2. Connected fallback with `Review recovery` and `Restore overlay` guidance.
3. Disconnected-display fallback with `Retry overlay` guidance.
4. Successful restore with the updated settings revision announcement.

The contract test checks the stable object names and copy used by these states;
native desktop screenshots remain required evidence for platform qualification.
