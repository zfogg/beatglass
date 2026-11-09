# PipeWire epoch admission

`MonitorBackend::accepts_epoch()` is the control-thread admission check for
captured blocks. A nonzero block epoch is accepted only when it matches the
currently selected active session epoch; zero and stopped/restarting sessions
are always rejected. This prevents
blocks already queued by a stopped or replaced stream from crossing a
restart/source boundary.

The check is observational and does not alter FIFO state. Call it before
publishing a popped block to downstream analysis.
