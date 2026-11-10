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
