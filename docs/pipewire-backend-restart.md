# PipeWire backend restart

`restart_stream()` is the control-thread reacquisition boundary. It forwards a
fresh nonzero epoch and negotiated format to the selected `MonitorSession`,
which replaces its bounded FIFO, clears callback error state, and marks the
first accepted block discontinuous. Native callback delivery remains closed
until this restart succeeds.
