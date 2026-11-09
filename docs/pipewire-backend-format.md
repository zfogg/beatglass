# PipeWire backend format

The backend exposes the negotiated sample rate and channel count through
`selected_format()`. It is populated only after a successful session start or
restart and becomes empty after reset, preventing consumers from treating a
requested format as an installed native stream format.
