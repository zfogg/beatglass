# PipeWire backend restart

`restart_stream()` is the control-thread reacquisition boundary. It forwards a
fresh nonzero epoch and negotiated format to the selected `MonitorSession`,
which replaces its bounded FIFO, clears callback error state, and marks the
first accepted block discontinuous. Native callback delivery remains closed
until this restart succeeds.

The Linux owner also stops its thread loop before releasing the PipeWire
context. This prevents a late dispatcher callback from observing destroyed
stream objects during teardown; the portable contract test checks this
lifecycle boundary even when PipeWire headers are unavailable locally.
