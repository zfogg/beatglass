# PipeWire Linux qualification artifacts

Store one dated directory per native run with this layout:

```text
<date>-<host>/
  build.txt
  ctest.txt
  daemon-status.txt
  discovery.txt
  stream-format.txt
  callback-trace.txt
  lifecycle-trace.txt
  limitations.txt
```

`build.txt` must include the commit, compiler, PipeWire version, and configure
command. `callback-trace.txt` must demonstrate bounded handoff; the lifecycle
trace must cover error, stop, restart, and source reacquisition. Empty or
missing artifacts keep the native qualification row unrun.
