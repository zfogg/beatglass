# Library double-click acceptance

Double-click activation in both the folder browser and recursive results now
consumes the pointer event after opening/selecting and, for audio files,
starting playback. This prevents the activation from leaking into an adjacent
control or causing a second action.

Portable coverage lives in
`tests/unit/check_issue76_library_doubleclick_acceptance_contract.py`.
