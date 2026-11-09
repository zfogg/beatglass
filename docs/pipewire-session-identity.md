# PipeWire session identity

`MonitorSession::start()` rejects empty, whitespace-only, and NUL-containing
sink serials before allocating a stream or replacing the current session. This
keeps source selection aligned with discovery’s identity validation and leaves
an active session untouched when a malformed selection is supplied.
