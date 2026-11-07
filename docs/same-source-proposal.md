# Proposed same-source file analysis boundary

This is a contract proposal, not an implemented cacheable file-analysis path.
The existing `FileReader` and replay open a path internally and cannot yet use
`ByteSource`. Fingerprinting that path separately does not establish that the
decoder saw the fingerprinted bytes, even if before/after hashes match.

A single job worker opens the original read-only and retains platform file
identity and version evidence. It creates a private, exclusively created,
permission-restricted snapshot using bounded buffers, hashing the exact bytes
written. After the snapshot is finished, it keeps all write-capable handles
private and closed or otherwise prevents modification for the entire analysis.
The job owns the read-only snapshot source until the synchronous decoder and
fingerprint borrowers have finished. A future libsndfile virtual-I/O adapter
borrows this same `ByteSource` and its own `ByteCursor`; it must not reopen the
original path. Reads and seeks are confined to the worker, never to an audio
callback or GUI thread. Snapshot creation, cleanup and OS handle conversion
are platform implementations, not guarantees supplied by the new interface.
The libsndfile adapter must catch all exceptions at each C callback boundary,
record read/seek/cancellation failure in adapter-owned state, and translate it
on the worker after the library returns. Signed seeks must reject underflow,
overflow and positions beyond EOF without moving the cursor.

Before publication the worker checks the retained original identity, contents,
version evidence and intended path binding; changed or unverifiable evidence
must reject publication. Cancellation and I/O failure must remain distinct.
A mutable original can change during copying and be restored, and a writer can
modify it after final verification and before or after commit. Ordinary stat,
hash, and path checks do not close these races. Each platform must document
its stronger locking or snapshot guarantees, residual races and filesystems
where it cannot prove the required evidence. A snapshot ensures the decoder
and fingerprint use the same bytes only if mutation of the snapshot itself is
prevented. No source files or tags are modified.

Before product integration, exercise actual decoder virtual I/O against a
secure snapshot and test path replacement, in-place modification/restoration,
copy interruption, short reads, seek/EOF, cancellation, failed verification,
temporary-file permissions and cleanup, and preservation of the prior cache
entry. Test native Windows, macOS and Linux identity behavior and document what
cannot be proved. The byte cursor unit tests cover positioning and malformed
read handling only; they do not prove any filesystem or decoder guarantee.
