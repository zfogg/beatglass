# Source, job and presentation handoff proposals

These proposals are for the integration owner to review before the foundation gate opens. They describe required behavior and contract gaps; they do not assert that the current interfaces implement it. No parallel feature handoff is authorized yet.

## Source and worker boundary

The file reader is worker-owned and synchronous: the caller supplies the interleaved float buffer and a unique initial epoch, and a successful seek increments that epoch. EOF and cancellation before I/O are explicit. The reader cannot interrupt a blocking filesystem read, so shutdown must not assume immediate cancellation. File samples have no live monotonic anchor. The current reader is separate from the callback-oriented `AudioSource`; the integration owner must agree how source discovery/control and pull reads fit together before calling the source contract stable.

A live backend owns start/stop and device transitions on its control thread. The callback writes only a bounded preallocated queue, including epoch, frame offset and timing/status; it never publishes a Qt object or allocates a message. The consumer detects overflow/gaps with `StreamContinuity`, resets estimators and invalidates snapshot tokens before accepting new evidence. Stop must quiesce callbacks before the source or its storage is destroyed. Permission denial, invalidation, unsupported format and interruption need typed control-thread status and tests; the present string status is not a sufficient error contract.

## Library jobs and persistence

Proposal: the job coordinator owns bounded admission and priorities; workers own decoding/DSP and observe a stop token at safe block boundaries; a database owner serializes writes. Progress events carry job/run identity and monotonically increasing completed frames. A total must distinguish unknown from a real zero-length file; the current `JobProgress::total_frames` cannot express that distinction. Proposed states distinguish queued, running, cancelled, failed and complete, with error details delivered off the GUI thread.

Proposal: cancellation observed before the database writer begins its final commit prevents publication; once that commit begins, the result reports the actual committed outcome. A cancelled, failed or interrupted run must not replace a completed active run. During shutdown, stop admission, request worker cancellation, join workers, then drain or roll back the writer before closing its connection. A decoder blocked inside OS I/O remains a concrete cancellation limitation.

Proposal for cache identity: hash content with a documented, versioned cryptographic hash when a file is selected for analysis/reuse, not for an entire collection at every launch. Include the hash, algorithm version, configuration and offline/live mode in the cache key. Read from an opened handle, record file identity and attributes before/after analysis, and validate the content that was analyzed before publishing. Files can change without a reliable attribute change; do not treat matching path/size/mtime alone as proof of content identity. Publication must be a transaction that retains the prior complete result until its replacement succeeds. Exact hashing, file-change race handling and a typed lookup/publish result (hit, miss, cancelled, changed, error) require an agreed header/schema; the current `optional`/`bool` interface does not distinguish all outcomes.

Required acceptance tests: unchanged content/config reuses a completed result without DSP; changed content or config does not; moved/duplicate content can reuse validated identity; cancellation, file mutation during analysis and a simulated restart never publish a partial or mismatched result. The integration owner must approve migrations and schema before the library owner depends on them.

## Presentation and publication

The analysis worker owns estimator state and creates immutable snapshots. A single worker owns `SnapshotPublisher`; it advances the acquisition token after invalidation and clears latest publication on stop, silence or error. The GUI reads the latest pointer on its own thread at a bounded repaint cadence and never accesses mutable estimator internals. An old shared pointer can outlive `clear()`, so a GUI that caches it must discard it on invalidation rather than presenting it as live. Beat/chord event histories supplied to the publisher must be ordered and bounded; full offline timelines belong in storage, not an unbounded live queue. Scores remain strengths, not calibrated probabilities.

The presentation interface currently leaves stale-age policy, error delivery, UI-facing timeline mapping and Qt ownership unspecified. Before handoff, define a monotonic expiry policy and test slow GUI readers, reset during publication, epoch changes and shutdown. The shared UI owner owns `src/desktop` models and QML; the integration owner owns shared result/header changes.
