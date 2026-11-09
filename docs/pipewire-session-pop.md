# PipeWire session pop

`MonitorSession::pop()` is the control-thread boundary for consuming bounded
PCM blocks. The native callback only copies into the preallocated FIFO through
`on_process`; the session owner then pops blocks and owns their lifetime. A
failed pop means the queue is empty or no session is active, not that callback
memory escaped to another thread.
