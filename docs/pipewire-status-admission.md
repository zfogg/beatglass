# PipeWire status admission

`MonitorStatus::accepts_epoch()` provides a side-effect-free check for
consumers holding a status snapshot. It rejects zero, stale, and inactive
epochs, so a snapshot cannot accidentally authorize data after stop or
restart.
