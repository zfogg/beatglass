# Overlay recovery workflow

The control window keeps one predictable recovery path visible whenever the native
overlay is unavailable or its persisted settings need attention.

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

## Screenshot qualification

When desktop rendering is available, capture these states at the standard desktop
viewport and attach the images to the UI qualification issue:

1. Ready overlay with the settings summary.
2. Connected fallback with `Review recovery` and `Restore overlay` guidance.
3. Disconnected-display fallback with `Retry overlay` guidance.
4. Successful restore with the updated settings revision announcement.

The contract test checks the stable object names and copy used by these states;
native desktop screenshots remain required evidence for platform qualification.
