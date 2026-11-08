# Provisional PipeWire monitor status contract

This is a private implementation contract for the experimental Linux monitor. It does not claim native Linux qualification or a stable public API.

The monitor remains `starting` until its initial graph is validated. It becomes `running` only after the ordered running event is consumed. Graph loss or capture discontinuity becomes `interrupted`; startup and backend failures become `error`. Every non-running state revokes the worker epoch, so retained analysis must be cleared before a new acquisition installs a fresh epoch.

Diagnostic values are provisional and are useful only for local tests and logs:

| Value | Meaning |
| ---: | --- |
| 12 | Startup deadline exceeded |
| 13 | Watched node, port, or link failed validation |

Product-facing code should use the source state and error category rather than these numbers. Native device, permission, compositor, overlay, and desktop qualification remain open requirements.
