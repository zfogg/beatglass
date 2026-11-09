# Persisted retry selection

The persisted retry action now reports an explicit rejection when invoked
without a selected retryable run. It also reports worker unavailability instead
of silently ignoring the action. This keeps the recovery workflow truthful and
leaves selection unchanged until a real retry request is queued.
