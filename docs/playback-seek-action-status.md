# Playback seek action status

The Library reports whether seek is unavailable because playback is stopped, the
open source differs from the selected source, or seek is ready. This keeps the
timeline action synchronized with playback/source identity and clarifies that
seeking uses source frames without modifying the audio file.
