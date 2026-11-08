# Storage owner contract

`StorageOwner` is the library-facing lifetime boundary around the SQLite
cache. The owner is intentionally small: it opens one cache connection,
exposes recovery queries, and closes the connection. It does not own source
audio and its recovery operations must never remove or modify the source file.

## Lifecycle

- `open` constructs the cache at the configured database path and returns
  `false` with an optional diagnostic if construction fails.
- A successful `open` replaces the prior cache connection. Callers must stop
  storage work before reopening or closing it.
- `close` releases the connection and is `noexcept`; after close,
  `retryable_runs` returns an empty list and `discard_retryable` returns
  `false`.
- `cache()` is an internal-worker escape hatch. It is null while closed and
  must not be used from GUI, audio-capture, or unrelated worker threads.

## Recovery ownership

`retryable_runs(limit)` reads persisted unfinished runs in deterministic ID
order, with the same bounded limit as the underlying cache. The caller keeps
the returned run identity and key components; the returned record is a
snapshot, not a reservation. A second reader may observe the same run, so a
retry coordinator must serialize retry and discard decisions on the storage
owner's worker.

`discard_retryable(id)` deletes only a retryable run record with the supplied
ID. It does not delete source audio, active completed results, or a run in any
other state. A `false` result means no matching retryable record was changed;
it is not a storage exception.

The owner does not decide whether a retry is safe. The analysis layer must
reconstruct a complete `CacheKey`, revalidate the source, and publish through
the cache transaction. Until that succeeds, the previous active result stays
authoritative.

## Threading and qualification boundary

The SQLite connection is worker-owned and all SQL must run on that worker.
The API does not make concurrent calls safe by itself; the integration owner
must provide the queue/serialization boundary when wiring it to desktop UI or
analysis jobs. This contract does not claim native audio or GUI qualification.
Those scenarios remain part of the issue #1 platform gates.
