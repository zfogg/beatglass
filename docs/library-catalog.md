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
