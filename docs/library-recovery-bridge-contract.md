# Persisted recovery bridge contract

`StorageRecoveryBridge` owns a dedicated QThread and keeps SQLite, cache,
decoder, and job-handle work on its nested worker. GUI callers only enqueue
operations through invokables; they must observe the result/status signals
instead of reading storage directly. `close()` clears GUI-visible selection,
cache, result, progress, and retry state before the worker close is processed,
so a database transition cannot expose records from the previous connection.

The bridge never mutates source audio. Retry and discard operate only on
persisted recovery records, and all terminal/error paths report through
`statusChanged`, `analysisStatusChanged`, or `analysisErrorChanged`.

The bridge is non-copyable because it owns a worker thread, queued invocations,
and GUI-visible state. Pass it by QObject ownership/reference; never copy it
between windows or threads.

`storageConfigured` is true only after a non-empty database path is injected;
it does not claim that SQLite is open. Consumers should continue to use
`statusChanged` for the asynchronous open/close result.

`storageOpen` is the corresponding asynchronous state: it becomes true only
after the worker reports `Storage open` and false after `Storage closed` or an
open failure. Configuration and open state are intentionally separate.
The bridge tracks an in-flight open/close transition so an open failure also
drives `storageOpen` false without misclassifying unrelated worker errors.
Injected database paths are trimmed before configuration; whitespace-only input
therefore leaves storage unconfigured instead of queuing an invalid open.
Replacing a configured path with an empty path also closes the previous worker
storage first, preventing a cleared configuration from retaining an open
database.
Overlapping open/close requests are rejected with `Storage transition already
in progress`; callers must wait for `storageTransitionPending` to clear.
Calling `open()` again while `storageOpen` is true is rejected with `Storage
already open`; callers must close or change the configured path first.
Calling `close()` while no storage is open reports `Storage already closed`
without enqueueing redundant worker work.
`storageTransitionPending` is true only while an asynchronous open/close is in
flight, allowing consumers to disable duplicate lifecycle actions safely.
`recoveryAvailable` is true when the latest worker refresh contains at least
one retryable persisted run; it is false after close, refresh failure, or an
empty recovery list.
The desktop recovery action gate consumes `recoveryAvailable` rather than
duplicating list-length logic.
Recovery guidance and persisted retry outcome status use the same property, so
stale availability text and controls disappear together after refresh, retry,
or discard clears the saved-run snapshot.
The detailed status description and retry control use this property as well,
so a stale run-list representation cannot keep recovery actionable after the
bridge has cleared its snapshot.
Changing the database path is subject to the same transition guard; path
ownership cannot change underneath an in-flight close or open.
Lifecycle failures and storage close/open resets clear both retryable run IDs
and their detail strings, keeping the GUI model from retaining descriptions for
an unavailable recovery snapshot.
Cancellation and retry of an analysis handle use the same storage-open guard,
rather than queueing work against a missing database.
Submission and admission also require a non-empty content identity before
touching the worker, keeping analysis and cache records addressable by the same
source identity contract.
Refresh requests made while storage is closed are rejected synchronously with
`Storage error: storage is not open`; callers must reopen storage before asking
the worker for a new persisted-recovery snapshot.
Cache lookup and invalidation follow the same boundary and reject synchronously
while storage is closed, preserving cache state until a configured database is
open.
Both operations also reject an empty content identity before touching storage,
preventing an accidental unscoped cache query or invalidation.
Analysis admission uses the same guard and reports `Analysis admission rejected:
storage is not open` until storage is available.
New analysis submissions are rejected with `Analysis rejected: storage is not
open` until the configured database has been opened.
