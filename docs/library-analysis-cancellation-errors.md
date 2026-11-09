# Library analysis cancellation errors

Cancellation is issued to the worker-owned job handle and then polled for its
terminal state. If that lifecycle boundary rejects cancellation, the bridge
now clears the stale handle and emits `Analysis cancellation rejected:` rather
than leaving the UI attached to an unknown job. A normal cancellation still
flows through the existing cancelled/retryable state and preserves the source
and cache records.
