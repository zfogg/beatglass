# PipeWire stream-owner integration seam

`stream_owner_linux.cpp` is the Linux-gated boundary for the native `pw_stream`
adapter. It deliberately keeps PipeWire and SPA headers out of portable
translation units. The eventual implementation must negotiate the requested
sample rate/channel count, resolve the selected serial to a target node, and
copy dequeued float PCM into the bounded `MonitorStream` handoff.

The callback contract forbids allocation, blocking, logging, UI/database calls,
or exposing PipeWire-owned memory. Linux native compilation and runtime
qualification remain pending until a host with `libpipewire-0.3` headers and a
running PipeWire session is available.

Native `ERROR`, `UNCONNECTED`, `PAUSED`, destroy, malformed-buffer, and bounded
handoff failures all quiesce callback admission and require a fresh epoch.
Negotiated formats that are not F32 or do not match the requested rate/channel
configuration report `-EPROTO` and follow the same reacquisition path.
