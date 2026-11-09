# Persisted recovery selection

Selecting a persisted retryable run now publishes `Persisted recovery
selected`, giving the recovery surface an explicit handoff state before the
user retries or discards the run. Clearing the selection remains silent so a
refresh can remove a run without creating a misleading selection event; an
explicit clear now publishes `Persisted recovery selection cleared`.
