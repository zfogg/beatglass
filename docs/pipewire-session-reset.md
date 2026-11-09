# PipeWire session reset

`MonitorSession::reset()` is the control-thread teardown boundary. It first
stops the stream to close callback admission, then releases the bounded stream,
selected sink identity, and installed format. A later start therefore requires
a fresh validated sink and nonzero epoch rather than reusing stale lifecycle
state.
