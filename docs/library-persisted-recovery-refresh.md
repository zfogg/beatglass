# Persisted recovery refresh

Refreshing persisted retryable runs now publishes `Refreshing persisted
recovery` while the storage worker reloads durable state. Worker-unavailable
errors remain explicit, so recovery controls do not silently appear stale.
When the worker returns its refreshed run list, the bridge publishes
`Persisted recovery refreshed` as the completion state.
