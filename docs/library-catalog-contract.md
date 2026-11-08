# Catalog browse contract

`library::browse` performs one synchronous, non-recursive directory query for
the Library view. It includes directories for navigation and supported regular
audio files, applies a case-insensitive substring filter to each entry name,
and does not follow child directories.

Successful results are deterministic: entries sort by folded name, then exact
name, then path. A filesystem failure returns an empty `entries` vector and a
diagnostic; callers must not mix a partial list with an error diagnostic. The
function never creates, deletes, tags, or otherwise modifies user files and
must run on a worker rather than an audio callback or GUI thread.

Catalog browsing is intentionally filesystem-only. Metadata, duration,
content identity, and decoder qualification belong to later worker stages and
must not be inferred from the catalog row alone.
