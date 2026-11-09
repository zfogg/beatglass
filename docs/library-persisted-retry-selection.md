# Persisted retry selection

The persisted retry action reports explicit rejection when invoked without a
selected retryable run and reports worker unavailability instead of silently
ignoring the action. When accepted, it publishes `Retrying persisted recovery`
until the worker returns the terminal retry result.
Before requeueing, the bridge clears stale displayed results and progress from
the previous attempt.
