# Analysis batch action status

The Library summarizes which batch recovery actions are currently meaningful:
active batches expose cancellation, while failed or cancelled batches expose
retry and discard. The status is read-only guidance; every action preserves the
original source audio and returns focus to the relevant control.
