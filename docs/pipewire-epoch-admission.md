# PipeWire epoch admission

`MonitorBackend::accepts_epoch()` is the control-thread admission check for
captured blocks. A nonzero block epoch is accepted only when it matches the
currently selected active session epoch; zero and stopped/restarting sessions
are always rejected. This prevents
blocks already queued by a stopped or replaced stream from crossing a
restart/source boundary.

The check is observational and does not alter FIFO state. Call it before
publishing a popped block to downstream analysis.

`MonitorSession::pop_current()` applies this admission rule while draining the
bounded callback queue: stale blocks from a previous stream epoch are consumed
and discarded, and only a block from the active epoch is returned to the
control thread. Native daemon qualification still requires a real Linux run;
portable contract tests cover the fail-closed boundary without claiming native
evidence.
