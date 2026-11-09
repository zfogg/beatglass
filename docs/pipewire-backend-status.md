# PipeWire backend status

`MonitorStatus` is the shared control-thread status value for the PipeWire
backend. It reports active-session state, restart demand, discontinuity,
bounded-queue overflow, and the consumed callback error code. The backend
forwards this value from its `MonitorSession`, keeping lifecycle recovery above
the native stream boundary.
