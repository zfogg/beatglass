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
The same action is available from the keyboard with `Ctrl+Alt+Shift+X` while
an active retry cycle is in progress.
The active recovery card repeats this shortcut next to the Stop recovery action
so pausing a retry remains discoverable.
After `Stop recovery`, the card keeps a persistent `Recovery paused · monitoring
continues` status so an intentional user pause is distinguishable from a native
bridge failure; the next retry remains an explicit action.
Focus moves directly to `Resume recovery` when the pause is requested, keeping
the next lifecycle action keyboard-ready instead of targeting a hidden control.
The paused card also exposes `Resume recovery`, which starts a fresh bounded
attempt without requiring the user to rediscover the retry control.
For keyboard-first recovery, `Ctrl+Alt+Shift+R` performs the same resume action;
the shortcut is repeated beside the paused action while it is available.
Pressing `Escape` while paused returns focus to `Resume recovery`, providing a
predictable keyboard path back to the lifecycle action after reviewing status.
It also announces that the paused recovery action is focused for assistive
technology users.
`Ctrl+Alt+Shift+P` opens the paused recovery page from anywhere in the
control window and focuses `Resume recovery` for a complete keyboard workflow.
The page shortcut is only active while the pause state is live and disappears
with its hint as soon as recovery is resumed.
If a display is disconnected and later restored while recovery is paused, focus
returns to `Resume recovery` rather than the ordinary retry control, preserving
the intentional pause across the display lifecycle.

## Screenshot qualification

When desktop rendering is available, capture these states at the standard desktop
viewport and attach the images to the UI qualification issue:

1. Ready overlay with the settings summary.
2. Connected fallback with `Review recovery` and `Restore overlay` guidance.
3. Disconnected-display fallback with `Retry overlay` guidance.
4. Successful restore with the updated settings revision announcement.

The contract test checks the stable object names and copy used by these states;
native desktop screenshots remain required evidence for platform qualification.
