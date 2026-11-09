# Library worker discard stale-result reset

Discarding worker-level analysis recovery now invalidates prior visualization
evidence after the discard operation. A new accepted analysis result is required
before the visualization is current again; source audio remains untouched.
