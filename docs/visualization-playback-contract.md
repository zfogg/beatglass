# Playback and visualization presentation contract

This contract defines the boundary for the future library-player visualization integration. It keeps source identity and timing explicit before a player drives the live visualization surface.

## Source identity

- Playback opens a source with a monotonically increasing `epoch`.
- Visualization frames carry the same source epoch through the presentation bridge's generation token.
- A source change, stop, seek discontinuity, capture error, or reacquisition clears the presentation and advances the generation. A delayed frame from an older generation is rejected.

## Ordering and bounded delivery

- Each frame has a strictly increasing `revision` within its generation.
- The GUI presentation accepts only a newer revision for the current generation; it copies bounded waveform and FFT data before notifying QML.
- The audio callback never calls the presentation bridge. Worker or coordinator code owns frame production and delivery; QML only paints the latest accepted snapshot.

## Timing and selection

- `first_frame`/`end_frame` are source sample positions, not wall-clock milliseconds. A player integration must provide the sample rate and epoch with every seek or frame request.
- The QML waveform/FFT selection is a presentation cursor only. It must not imply a seek until a playback bridge contract explicitly maps the selected position to a validated source frame.
- Unknown duration, decoder discontinuity, and unavailable output must remain visible states; no stale frame may be presented as current evidence.

## Required integration tests

The bridge integration must cover: accepting a current epoch/revision, rejecting an old epoch, rejecting a duplicate revision, clearing on seek/stop, and retaining bounded data after an accepted update. Existing `visualization_presentation_test` coverage establishes the generation/revision gate; playback integration should add epoch mapping tests without bypassing that gate.
