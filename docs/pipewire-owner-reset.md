# PipeWire owner reset

The control-thread `DiscoveryBoundary` now owns the native `StreamOwner` and
quiesces it before resetting the monitor session. This establishes the safe
destruction order required by the eventual Linux `pw_stream` implementation;
native callback/runtime qualification remains unrun on Windows.

The same quiesce step runs on ordinary backend stop, before session stop,
ensuring native callbacks cannot race the session's stopped state.
