# PipeWire backend session

The PipeWire backend now has a negotiated-format `start_stream` seam. Control
code must supply the selected serial, fresh epoch, sample rate, and channel
count before the bounded `MonitorSession` is allocated. This avoids inventing a
format and keeps native stream negotiation ahead of PCM delivery; stop routes
through the session owner and closes callback admission.
