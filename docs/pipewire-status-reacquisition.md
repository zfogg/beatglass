# PipeWire status reacquisition

`MonitorStatus::requires_reacquisition()` centralizes the lifecycle gate for
downstream consumers. Inactive, restarting, discontinuous, or overflowed
statuses require reacquiring the stream before publishing more audio.
