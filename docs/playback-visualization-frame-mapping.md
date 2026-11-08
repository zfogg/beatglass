# Playback frame mapping contract

The library player and visualization presentation use integer source sample frames as their shared timeline. Wall-clock milliseconds are display-only and must not be used as the authoritative seek coordinate.

- A playback source provides a positive sample rate and a monotonically increasing source `epoch` when opened.
- `first_frame` is inclusive and `end_frame` is exclusive. A valid frame window satisfies `end_frame > first_frame`; the duration is `(end_frame - first_frame) / sample_rate` seconds.
- Converting a UI position to a source frame uses `floor(position * duration_frames)`, clamps to `[0, duration_frames]`, and never rounds past the exclusive end.
- A seek increments the playback epoch before requesting new visualization data. The presentation generation is cleared before delivery, so old frames cannot reappear after the seek.
- Unknown duration must leave seeking disabled or explicitly provisional; it must not guess from the number of currently visible waveform samples.
- Pausing preserves the current epoch and visualization frame. Stop, source replacement, decoder discontinuity, and capture reacquisition clear the frame and invalidate queued deliveries.

The integration test matrix must cover zero/unknown duration, exact start/end positions, fractional positions, a seek across an epoch change, and rejection of a delayed frame from the previous epoch.
