# Library persisted retry stale-result reset

Retrying a selected persisted analysis run now invalidates prior visualization
evidence after retry admission. The selected run remains focused while fresh
analysis completes, preventing retained output from appearing current.
