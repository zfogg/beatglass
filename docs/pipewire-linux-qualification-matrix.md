# PipeWire Linux qualification matrix

| Scenario | Required observation | Evidence |
| --- | --- | --- |
| Discovery | Sink serial is returned and timeout is bounded | discovery log |
| Start/format | `pw_stream` connects at the negotiated rate/channels | build log + status |
| Process | Callback reaches bounded `MonitorStream` without allocation | capture trace |
| Error | Native error appears in the next status poll | error log |
| Stop | Owner quiesces before session stop | lifecycle trace |
| Restart | Fresh epoch and discontinuity are observed | status + capture trace |
| Reacquire | Old source is reset before the new source connects | lifecycle trace |

Every row must be marked pass/fail with the host, commit, and artifact path.
An unrun row keeps native qualification incomplete.
