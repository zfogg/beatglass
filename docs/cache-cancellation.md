# Cache cancellation safety

Cancellation during `publish_file_result` abandons only the in-flight run.
It must not invalidate the prior completed cache row: cancellation is not
evidence that the source changed or became unreadable. Invalidation remains
reserved for a verified content-identity change. This preserves a usable
previous result while allowing the work to be retried later.

Retryable-run enumeration also accepts a stop token. If shutdown or a newer
recovery request arrives during the SQLite iteration, it throws
`CollectionCancelled` and leaves every retryable row unchanged.
The targeted `retryable_run(id)` lookup follows the same rule before and after
its SQLite step.

Recovery/UI code can use `fingerprint_status_label` for the stable labels
`ok`, `cancelled`, `changed`, and `error`; diagnostics remain supplemental
detail rather than machine-readable keys.

Only `cancelled` and `error` are retryable outcomes. A `changed` result means
the source identity must be refreshed before another attempt; it is not a
transient failure and must not be blindly retried against the old identity.

Callers can use `fingerprint_status_requires_refresh` to make this distinction
explicit: only a `changed` result requires refreshing the cached identity;
`cancelled` and `error` remain retryable without discarding a known identity.

For user-facing diagnostics, `fingerprint_status_diagnostic` prefixes optional
detail with the stable status label (for example, `error: cannot open`).

The cache identity implementation uses the same predicate, so all publication
and collection paths invalidate only after a verified `changed` outcome.
Collection pruning reports that outcome as `changed: content identity changed;
catalog entry removed`, making the destructive action explicit to callers.

The cache-identity API clears supplied diagnostic strings before each operation;
on non-success they contain the stable status label and optional detail, while
matching observations leave the diagnostic empty.
