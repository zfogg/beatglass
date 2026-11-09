# PipeWire status epoch

`MonitorStatus` now carries the nonzero capture epoch associated with the
selected sink and negotiated format. The epoch changes on successful start or
restart and clears on reset, allowing consumers to reject stale blocks and
attribute discontinuities to the correct lifecycle generation.
