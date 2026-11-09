# Persisted discard selection

The persisted discard actions now report explicit rejection when invoked
without a selected retryable run or batch. They also report worker
unavailability instead of silently ignoring discard. Destructive recovery
actions therefore remain scoped to explicit selections.
When a batch is accepted, the bridge publishes `Discarding persisted recovery`
until the worker reports the terminal result.
Single selected discard publishes the same transition status as batch discard.
It clears stale displayed result and progress before requesting the destructive
recovery operation.
Batch discard applies the same stale-result and progress reset before queueing.
Both discard paths also clear stale retry/error state before the destructive
operation begins.
When batch discard completes successfully, the bridge returns the analysis
status to `Analysis idle` while the refreshed run list removes discarded rows.
