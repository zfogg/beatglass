# Library duration contract

The filesystem-only catalog does not decode audio. A catalog row therefore reports `durationSeconds = -1` until a metadata or analysis worker has supplied decoded duration evidence. The Library UI must render that value as unknown and must not infer duration from file size, waveform length, frame counts, or a playback block count.

Once a worker publishes duration, it must be attributed to the same source identity and file version as the row. A moved, changed, missing, unsupported, or unreadable source clears the published duration back to unknown. This keeps the UI from presenting stale timing as verified metadata.

The contract is intentionally separate from seekability: an unknown duration does not imply that a source is seekable, and a seekable source does not imply that decoded duration is available. Playback controls must retain their explicit unavailable/unknown state until both capabilities are reported.
