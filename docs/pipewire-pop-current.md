# PipeWire current-epoch pop

`MonitorBackend::pop_current()` is the control-thread handoff for downstream
capture. It drains popped blocks until one whose immutable FIFO epoch matches
the active session epoch, preventing queued data from an old stream from
crossing a restart or source boundary.
