# Library recovery contract

This document records the recovery behavior implemented by the SQLite library
prototype. It is intentionally narrower than the product cache contract: it
describes durable run state and retry ownership, not source identity or native
qualification.

## Run state

`SqliteCache::begin` creates a run in `running` state. A run becomes
`complete` only when `publish` has validated the cache key and source, written
the serialized result, and committed the active-result transaction. A caller
must not infer completion from a run being accepted or from a result being
serialized in memory.

If publication cannot complete, the run must remain recoverable. The cache
uses `retryable` for that state. `mark_retryable` changes only the matching
`id` and complete `CacheKey` while the row is still `running`; it clears any
partial result and does not delete the run or change another run with the same
source identity. This makes recovery idempotent and keeps retry ownership with
the caller that owns the run ID.

## Failure boundaries

The current publication path catches serialization failures and attempts to
mark the begun run `retryable` before returning `error`. A failure while the
cache transaction is being committed is handled by the cache transaction
boundary and must not be reported as a successful replacement without commit
evidence. Cancellation observed by the worker before commit returns
`cancelled`; cancellation after commit has started cannot revoke a committed
replacement.

The separate pre-serialization cancellation change in PR #578 remains held
pending a rebuilt-binary regression test on the constrained Windows host. It
is not part of the behavior claimed by this document or by `origin/main`.

## Recovery workflow

Consumers should:

1. retain the run ID returned by `begin`;
2. treat `retryable` as unfinished work, never as a cache hit;
3. query/list retryable runs after reopening the cache;
4. retry or explicitly discard the run through the cache API; and
5. preserve the previous active completed result until a new publication is
   committed.

Recovery does not establish content identity. The caller still has to provide
the validated content hash, engine version, configuration ID and analysis mode
in the `CacheKey`, and publication still has to revalidate the source through
its verifier. The prototype's opaque serialized result and host-provided
SQLite dependency remain provisional until the integration owner accepts the
Phase 1 schema and dependency decisions.

## Validation on constrained development hosts

The contract can be checked without compiling the SQLite amalgamation:

```powershell
git diff --check origin/main...HEAD
rg "mark_retryable|retryable|publish_operation" include lib tests docs
```

## Bridge-visible recovery

The desktop analysis bridge keeps terminal failure and cancellation actionable:
the status remains `Failed` or `Cancelled`, stale result text is cleared, and
`canRetry` stays true while the retained operation is available. The failure
diagnostic explicitly says that retry is available, so a UI consumer does not
need to infer recovery from a generic terminal label. Batch failures and
cancellations expose the same guidance. Native screen-reader timing remains a
qualification responsibility; the portable bridge contract is checked with
`python tests/unit/check_bridge_recovery_action_contract.py`.

Terminal recovery can also be discarded explicitly. Single-job `discard()`
and batch `discardBatch()` clear the retained handles, displayed result,
diagnostic, progress, counters and terminal status, returning the bridge to
`Idle`. They never remove source audio or mutate persisted library records;
discarding a persisted retry remains a storage-owner operation.

The focused CTest remains the authoritative executable regression. On the
current Windows development host it is not claimed here because rebuilding
the modified SQLite test binary has exhausted available memory and has also
encountered executable permission locks. That limitation is tracked with the
held cancellation PR rather than hidden by a source-only pass.
