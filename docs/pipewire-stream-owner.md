# PipeWire native stream owner

`detail::StreamOwner` is the source-level seam for Linux native `pw_stream`
ownership. Its control-thread `connect()` receives the selected sink and
negotiated format and binds callback delivery to the bounded `MonitorStream`;
`stop()` must quiesce callbacks before destruction.

The concrete owner remains blocked on Linux PipeWire headers and a live daemon.
Windows validation covers only this seam contract; Linux compile, callback,
and runtime qualification remain explicitly unrun.
