# PipeWire session status

`MonitorSession::poll_status()` is the control-thread lifecycle poll. It
returns whether a stream is active, whether it needs a fresh restart, whether
the callback published a discontinuity, whether bounded storage overflowed, and
the latest callback error code. Error and discontinuity markers are consumed
once per poll; the callback path remains allocation-free and nonblocking.
