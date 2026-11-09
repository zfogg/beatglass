# Library batch progress empty state

When no batch items have been accepted, the progress control now reports that
explicitly instead of exposing a generic unavailable value. Active batches still
report their in-progress state and detailed completion counters.
