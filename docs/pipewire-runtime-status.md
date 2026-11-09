# PipeWire runtime status

The experimental `beatglass_pipewire` target now builds both registry
discovery and the control-thread `MonitorBackend` adapter. This is the honest
integration seam available on the current branch: `source_adapter.cpp` owns
the bounded FIFO/session lifecycle, while native `pw_stream` creation,
format negotiation, and process/state callback registration remain the next
Linux-only implementation step.

This worktree is Windows and does not have `libpipewire-0.3` headers or
pkg-config metadata, so native compilation/runtime qualification cannot be
performed here. Linux qualification must run on a host with PipeWire
development packages and a live daemon.
