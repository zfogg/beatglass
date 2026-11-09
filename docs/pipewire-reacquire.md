# PipeWire source reacquisition

`MonitorBackend::reacquire()` is the explicit control-thread handoff for a
source boundary. It resets the old session before starting the selected sink
with a fresh epoch and negotiated format, ensuring callbacks from the prior
source cannot remain admitted.
