# Library play shortcut workflow

Ctrl+Alt+P now routes through the shared selected-track playback lifecycle
instead of opening the source directly. This keeps stale-visualization reset,
playback recovery announcements, and keyboard focus consistent with the
visible Play selected control.

The workflow is covered by
`tests/unit/check_issue76_library_play_shortcut_contract.py`.
