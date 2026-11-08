# File fingerprint contract

`fingerprint_file` is a worker-side evidence helper, not a substitute for
capturing identity from the file handle used by a decoder.

On `ok`, the result contains the byte count, the file-clock modification time,
and a lowercase `sha256:` digest for the bytes read. The implementation reads
with a bounded buffer and never writes the source. A caller may use the
optional observer for synchronous progress after each nonempty chunk; it must
not retain the callback or assume that blocking filesystem I/O is
interruptible.

The result is `cancelled` when the stop token is observed before or during the
read. It is `changed` when the bytes exceed the initial size, the final byte
count is short, or the final size/mtime differs. Open, read, and digest
failures are `error` with a diagnostic when the underlying exception provides
one. These non-`ok` outcomes must not be used as cache identity.

The before/after size and timestamp checks reduce accidental races but cannot
exclude replacement, mutation, or ABA races, and they do not prove that a
separately opened decoder consumed the same bytes. Analysis publication must
revalidate the source at its own commit boundary and preserve the prior active
result when evidence is stale or unverifiable.
