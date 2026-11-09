# PipeWire backend pop and status

The PipeWire backend now forwards the control-thread `pop()` boundary from its
selected `MonitorSession`. Consumers can drain bounded PCM without reaching
into callback-owned memory or native stream objects; lifecycle status remains
available through the session owner.
