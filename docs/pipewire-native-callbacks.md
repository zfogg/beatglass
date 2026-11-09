# PipeWire native callback binding

`detail::NativeCallbackBinding` is the narrow, allocation-free handoff that a
Linux `pw_stream` process/state listener can use once PipeWire headers are
available. Process callbacks forward bounded sample spans to `MonitorStream`;
error callbacks only publish the native error and never allocate or block.

This Windows worktree validates the binding contract only. Linux compilation
against `libpipewire-0.3`, real callback registration, and runtime
qualification remain unrun.
