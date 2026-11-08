# Provisional PipeWire monitor status contract

This is an implementation contract for the private Linux monitor. It is not a stable public API and does not claim native Linux qualification.

The monitor reports `starting` until the initial graph and all watched nodes, ports, and links are validated. It reports `running` only after the ordered running event is consumed by the control owner. A watched graph change or capture loss reports `interrupted`; startup or backend failure reports `error`. Every non-running state revokes the worker epoch, so retained analysis must be cleared and a fresh acquisition attempt must install a new epoch before accepting audio.

The current diagnostic codes are intentionally numeric and provisional:

| Code | Meaning |
| ---: | --- |
| 1 | Stream explicitly disconnected |
| 3 | Duplicate format notification |
| 4 | Malformed or discontinuous ingress |
| 5 | Ingress overflow |
| 6 | Selected graph became invalid |
| 7 | PipeWire core error |
| 8 | Registry synchronization failed |
| 9 | Unexpected control callback failure |
| 10 | Owned link creation failed |
| 11 | Stream error callback |
| 12 | Startup deadline exceeded |
| 13 | Watched node, port, or link failed validation |
| 20–24 | Missing or unsupported stream format parameter |

The numeric values are useful for local diagnostics and regression tests only. Product-facing code should use the source state and error category, because the values may change while the monitor remains private. Native desktop, audio-device, permission, compositor, and overlay qualification remains outstanding.
