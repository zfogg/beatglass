# Library analysis admission recovery guidance

Analysis admission status now maps lifecycle states to actions: queued or running
work can be cancelled, while terminal failures can be retried or discarded. This
helps assistive technology choose the valid recovery path without guessing from a
status label alone.
