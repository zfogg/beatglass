# Library stale-result gating

Analysis presentation is source-attributed in the Library UI. A worker or
persisted result is rendered only when its source path matches the selected
audio source. Selecting another file immediately hides the previous result and
announces that fresh analysis is required; cancellation and retry retain their
existing focus-return behavior.
