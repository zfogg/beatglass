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
