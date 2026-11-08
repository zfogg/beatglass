# Storage open contract

`StorageOwner::open` is a worker-side replacement operation. It first closes
the previous cache, validates the supplied `StorageConfig`, and then attempts
to construct the SQLite cache. Any failure leaves the owner closed; callers
must check the boolean result before using `cache()` or recovery methods.

When a diagnostic pointer is supplied, failed validation or SQLite
construction writes the new failure diagnostic. The diagnostic is
informational and may be platform- or SQLite-specific; callers should use the
boolean result for control flow. Clearing stale text before a successful open
is tracked separately in held PR #619 and is not claimed by this document.

Opening does not create a platform default path or transfer ownership of the
parent directory. The storage worker owns the resulting connection, and
callers must finish storage work on that worker before calling `open` again or
`close`.
