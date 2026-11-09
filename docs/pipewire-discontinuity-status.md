# PipeWire discontinuity status

The control thread can consume the pending discontinuity marker exactly once
with `MonitorStream::take_discontinuity()`. Stop, callback error and successful
fresh-epoch restart publish the marker; the callback also consumes it when
annotating the next bounded PCM block. Both paths use atomic exchange so a
callback and control-thread observation cannot race or duplicate the marker.
