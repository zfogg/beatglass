# PipeWire backend identity

The control-thread backend exposes the selected sink serial through
`selected_serial()`. It returns an empty value after reset and the validated
identity used by the active session otherwise, allowing recovery and UI layers
to attribute status without inspecting native stream objects.
