# PipeWire status context

`MonitorStatus` carries the selected sink serial and negotiated format alongside
active/restart, discontinuity, overflow, and callback-error state. This keeps
control-thread recovery and UI attribution tied to the same session snapshot;
reset clears the identity and format rather than leaving stale context visible.
