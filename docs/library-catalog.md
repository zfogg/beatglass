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

## Bounded enumeration

`browse` and `browse_recursive` accept an optional `max_entries` budget (the
default is 10,000). The budget counts visible directory and supported-audio
entries after filtering. If enumeration would return more entries than the
budget, the result is fail-closed: `entries` is empty and `diagnostic` is
`catalog entry limit exceeded`. Callers can surface that diagnostic and ask
the user to narrow the folder or search instead of rendering a partial list.
