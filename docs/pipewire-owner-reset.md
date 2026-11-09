# PipeWire owner reset

The control-thread `DiscoveryBoundary` now owns the native `StreamOwner` and
quiesces it before resetting the monitor session. This establishes the safe
destruction order required by the eventual Linux `pw_stream` implementation;
native callback/runtime qualification remains unrun on Windows.
