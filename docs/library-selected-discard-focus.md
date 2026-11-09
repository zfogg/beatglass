# Library selected recovery discard

Discarding one saved retry record is a destructive recovery action, but it never
modifies source audio. The action now announces that scope before opening its
confirmation dialog. Existing dialog focus behavior keeps Cancel first and
returns focus to Discard saved run after either decision.
