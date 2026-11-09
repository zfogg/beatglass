# PipeWire stream thread-safety

The PipeWire process callback and control-thread lifecycle share only atomic
state for discontinuity publication. The callback consumes the pending marker
with an atomic exchange while the control thread publishes it with release
stores during stop and restart. FIFO ownership remains control-thread-only;
callback code only performs the bounded nonblocking push.
