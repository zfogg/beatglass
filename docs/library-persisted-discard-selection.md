# Persisted discard selection

The persisted discard action now reports an explicit rejection when invoked
without a selected retryable run, and reports worker unavailability instead of
silently ignoring the action. This keeps destructive recovery actions scoped
to an explicit selection and leaves the persisted run untouched when no valid
selection exists.
