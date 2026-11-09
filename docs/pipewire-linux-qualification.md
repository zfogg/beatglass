# PipeWire Linux qualification checklist

The Windows worktree validates only source contracts. Before claiming native
PipeWire support, a Linux host must record:

1. `pkg-config --modversion libpipewire-0.3` and the resolved
   `pipewire/pipewire.h` include path.
2. Successful `BEATGLASS_BUILD_PIPEWIRE=ON` configure/build with the
   `pw_stream` owner compiled under `BEATGLASS_HAS_PIPEWIRE=1`.
3. A live PipeWire daemon and a real output sink selected by serial.
4. Native stream format negotiation, process callback delivery into the
   bounded `MonitorStream`, callback error propagation, stop/quiesce, and
   fresh-epoch reacquisition.
5. CTest results plus captured logs/evidence for each scenario.

Until those steps are run on Linux, native PipeWire qualification is unrun.
