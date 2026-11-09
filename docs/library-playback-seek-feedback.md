# Library playback seek feedback

The visible five-second seek controls now announce successful backward and
forward seeks and preserve focus on the activated control. Stopped playback
continues to report that seeking is unavailable.

Portable coverage lives in
`tests/unit/check_issue76_library_playback_seek_feedback_contract.py`.
