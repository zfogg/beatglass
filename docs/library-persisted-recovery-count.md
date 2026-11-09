# Library persisted recovery count

Persisted recovery status now reports when retryable runs exist and tells users to
select one before retrying or discarding. The empty state remains available when
no retryable records exist, keeping recovery actions scoped to an explicit run.
