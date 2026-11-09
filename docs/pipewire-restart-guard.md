# PipeWire restart guard

`MonitorSession::restart()` now only replaces a stream after the existing
stream has reported a stop or callback error. This keeps an active callback
handoff from being replaced by an accidental duplicate restart and preserves
the control-thread lifecycle boundary.

The stream-level restart seam applies the same guard, so callers cannot bypass
the session lifecycle invariant.
