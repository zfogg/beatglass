# File-details contract

`library::inspect_file` is a read-only filesystem query for a caller-selected
audio path. It returns an owned `FileDetails` snapshot only when the path is a
regular file with a supported extension and both its modification timestamp
and byte size can be read. A missing path, directory, unsupported extension,
timestamp failure, or size failure returns `nullopt` rather than a partially
populated record.

The returned path preserves the caller's spelling, `name` is the final path
component, `extension` is lowercase, and `size_bytes`/`modified_ns` reflect
the observations made by the query. The function does not decode audio,
create or remove files, change metadata, or transfer ownership of the source.
Callers needing content identity or decoder attribution must perform the
stronger worker-side verification required by the analysis publication
contract; file details alone are not cache identity.
