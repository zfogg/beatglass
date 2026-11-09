# Library batch retry admission

The Library's batch recovery action now reports whether retry admission actually
succeeded. `AnalysisJobBridge::retryBatch()` returns `true` only when a retained
operation accepts the retry; a stale terminal state returns `false` without
starting work. The QML action announces the distinction so keyboard and screen
reader users are not told that a retry was queued after a race or exhausted
operation. The worker remains the owner of operation state and retry semantics.
