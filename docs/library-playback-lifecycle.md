# Library playback lifecycle

Open, Start native, and Stop playback actions now announce their lifecycle
result through the existing recovery status surface and preserve focus on the
activated control. This keeps source-open, native-output, and stopped states
reviewable in one keyboard workflow.

Portable coverage lives in
`tests/unit/check_issue76_library_playback_lifecycle_contract.py`.
