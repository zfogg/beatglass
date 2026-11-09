# Library playback timeline focus

Pause/resume and explicit frame-seek actions now preserve keyboard focus on
the activated control. Frame seeks announce the requested target, while pause
and resume continue through the shared playback lifecycle path.

Portable coverage lives in
`tests/unit/check_issue76_library_playback_timeline_focus_contract.py`.
