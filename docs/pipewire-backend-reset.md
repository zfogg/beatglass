# PipeWire backend reset

The backend exposes a control-thread `reset()` boundary that forwards complete
session teardown. Reset closes callback admission, releases the bounded stream,
selected sink identity, and negotiated format, so a later start cannot reuse
stale capture state.
