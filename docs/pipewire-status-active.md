# PipeWire active status

The control-thread status poll reports `active` only while a session exists and
does not require restart. Stopped, errored, overflowed, or otherwise invalid
streams remain observable through the status fields but are not presented as
actively tracking output.
