# Library format boundaries

The library exposes two deliberately different filesystem queries:

| API | Accepted regular-file extensions | Purpose |
| --- | --- | --- |
| `library::browse` | WAV, FLAC, MP3, AIFF/AIF | Fast, non-recursive navigation for the Library view |
| `library::inspect_file` | WAV, FLAC, MP3, OGG, AIFF/AIF, M4A | Direct metadata/detail lookup for a caller-selected path |

Both comparisons are case-insensitive and neither operation decodes audio or
modifies the source. `browse` includes directories so the UI can navigate;
`inspect_file` accepts only a regular file and returns `nullopt` for an
unsupported or unreadable path.

The narrower browse set is intentional: adding a decoder-supported extension
to direct inspection does not automatically make it a Library navigation
entry. Format admission for browsing must be coordinated with the catalog,
metadata and playback owners rather than inferred from `inspect_file`.
