# Library catalog format policy

`beatglass::library::audio_format_for_path` is the single filename-policy
boundary used by folder browsing. It lower-cases the final extension and
returns the canonical display label `WAV`, `FLAC`, `MP3`, or `AIFF` (including
`.aif`). Unsupported extensions return an empty string and are excluded from
catalog results.

The helper is pure: it does not inspect, open, follow, or modify the path.
Callers that need filesystem metadata must perform that work on a non-audio
worker. Symlink traversal and recursive enumeration remain governed by the
catalog functions, not this extension policy.

Each `CatalogEntry` also carries `relative_path`, computed from the query
root. It is suitable for persisted selection keys and display-state caching:
unlike `path`, it contains no machine-specific drive or absolute-root prefix.
For recursive scans it preserves descendant components (for example,
`nested/track.wav`). It is an identity hint, not permission to access a file;
callers must continue using the absolute `path` for filesystem operations.

## Cancellation

`browse` and `browse_recursive` accept an optional C++20 `std::stop_token`.
Workers should pass the token for a user-cancelled refresh or shutdown. The
implementation checks it before filesystem work and between entries; when
requested it discards any partial snapshot, sets `CatalogResult::cancelled`,
and returns the diagnostic `catalog query cancelled`. Cancelled results never
look like a successful partial library listing.

## Bounded enumeration

`browse` and `browse_recursive` accept an optional `max_entries` budget (the
default is 10,000). The budget counts visible directory and supported-audio
entries after filtering. If enumeration would return more entries than the
budget, the result is fail-closed: `entries` is empty and `diagnostic` is
`catalog entry limit exceeded`. Callers can surface that diagnostic and ask
the user to narrow the folder or search instead of rendering a partial list.

For programmatic handling, `CatalogResult::status` reports `CatalogStatus::ok`,
`invalid_root`, `filesystem_error`, or `entry_limit_exceeded`. The diagnostic
remains intended for user-facing detail and is not a stable error key.
