# Library analysis cancellation errors

Cancellation resets displayed progress before requesting stop on the worker
handle. The bridge then polls the terminal state and preserves the existing
cancelled/retryable lifecycle. If cancellation is rejected, it clears the
stale handle and emits `Analysis cancellation rejected:` instead of leaving an
old progress value attached to an unknown job.
The bridge publishes `Analysis cancelling` while the stop request is being
processed, before the terminal cancelled state is observed.
