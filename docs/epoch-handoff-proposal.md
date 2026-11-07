# Proposed process-session epoch handoff

This is a proposed integration contract, not a claim that the current desktop
already provides process-wide unique epochs. The Windows `DjBridge` now takes
an allocator from the application owner; the collection path must use that
same object before their publications can share a process-wide namespace.

One possible serialized handoff is for the application control owner to create exactly one session allocator before any
live or offline worker and retains it until every worker, source callback and
queued publication has quiesced. It serializes issuance and passes a fresh,
nonzero token to the relevant worker for every source start, restart, seek and
discontinuity; failed or cancelled attempts consume their token. If a worker
detects a discontinuity, it requests another token through its control handoff,
clears old evidence and remains stopped or reacquiring until the response
arrives. Neither the capture callback nor its bounded copy path calls the
allocator, locks, waits or allocates. Exhaustion invalidates the active result,
reports an error and prevents start or restart; it never wraps or reuses an
epoch. Old-generation replies must be rejected after stop or replacement.

An atomic compare-and-swap allocator shared by all workers can prevent duplicate
issuance if its lifetime, exhaustion and off-callback use are enforced. It is
an alternative worker-side issuance contract, not the control-owner token
handoff above; agree on that distinction before depending on it. Atomic
uniqueness does not order independent publications or make an old result
current: the existing generation, revision and invalidation gates remain
necessary. Concurrent tests must cover uniqueness, exhaustion and teardown.

The Windows integration adopts the worker-side alternative using the allocator
merged in PR #63: the GUI application constructs one allocator before
the bridge, passes it by reference, and destroys the bridge and joins its worker
before the allocator is destroyed. The worker's existing serialized follower
uses that reference for every live restart; no capture callback issues tokens.
An opt-in Windows test concurrently issues tokens from a simulated offline
worker during real loopback startup and bridge destruction; another seeds the
allocator at exhaustion and verifies no live snapshot is delivered. These do
not establish that an actual offline product path has adopted the allocator.
The implementation depends on review and integration of the offline seek
contract; it must not be advertised as complete process-wide uniqueness yet.
The noninteractive Windows bridge regression also opens a real generated WAV
with a coordinator-issued initial epoch, starts and restarts a failed live
endpoint with that same allocator, and verifies the reader block retains its
epoch while live attempts consume distinct values. This test deliberately does
not use the legacy seek overload. PR #45 proposes an explicit new-epoch seek;
that reader change and a combined library/live product integration are still
required for coordinated seeks.
The collection integration in #51 independently introduces an application-level
allocator. Before combining these branches, reconcile them into exactly one
object outside platform and feature conditionals, inject it into both bridges,
and destroy and join both bridges before the allocator. Do not merge two
separately owned allocators into the combined application.

The file reader currently increments its own epoch on seek; merely injecting a
shared initial value does not prevent it colliding with an epoch issued to
another source. Its seek contract must be changed to receive a fresh allocated
token, or otherwise use the same agreed allocator outside the callback, before
it participates in this process-wide namespace. A restarted process also needs
a distinct session identity or a proven drain of all prior publications. A
seeded allocator is safe only when the seed is a trustworthy high-water mark
for the same namespace; resetting or persisting it alone is not such a proof.
