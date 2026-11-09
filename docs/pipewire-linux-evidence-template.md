# PipeWire Linux evidence record

Copy this template for each native qualification run.

- Host/build identity: `<machine> / <commit> / <compiler>`
- PipeWire version: `<pkg-config output>`
- Header/library paths: `<resolved paths>`
- Daemon status: `<systemctl --user status pipewire>`
- Sink serial and negotiated format: `<serial> / <rate Hz / channels>`
- Process callback evidence: `<log or capture artifact>`
- Error/stop/reacquire evidence: `<log or capture artifact>`
- CTest command and result: `<command / pass-fail>`
- Limitations or unrun scenarios: `<details>`

Do not mark native qualification complete without all fields and attached
artifacts for the callback, stop, and reacquisition scenarios.
