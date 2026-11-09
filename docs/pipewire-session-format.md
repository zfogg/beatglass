# PipeWire session format

`MonitorSession::format()` exposes the sample rate and channel count installed
for the active control-thread epoch. The value is updated only after a
successful start or restart, so consumers cannot mistake a requested format
for one that is actually installed. A failed restart leaves the previous
format and stream status observable for recovery.
