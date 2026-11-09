# Persisted discard selection

The persisted discard actions now report explicit rejection when invoked
without a selected retryable run or batch. They also report worker
unavailability instead of silently ignoring discard. Destructive recovery
actions therefore remain scoped to explicit selections.
