# PipeWire selected epoch

`MonitorBackend::selected_epoch()` exposes the epoch currently owned by the
selected control-thread session. It is zero before a stream is started and
after reset. Consumers can compare this value with a captured block's epoch
before publishing samples, preventing blocks from an older stream from being
accepted after a restart or source change.

The accessor is observational and does not synchronize or mutate the stream;
call it on the control thread alongside `poll_status()` and `pop()`.
