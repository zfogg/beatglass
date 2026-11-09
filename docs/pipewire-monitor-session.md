# PipeWire monitor session

`MonitorSession` is the control-thread owner for one selected output sink and
one nonzero capture epoch. It validates the sink identity and format before
allocating a bounded `MonitorStream`, forwards native process callbacks without
allocating, and exposes stop/restart status for reacquisition. Native PipeWire
object creation remains an adapter concern; this boundary does not fabricate
PCM or claim native qualification.
