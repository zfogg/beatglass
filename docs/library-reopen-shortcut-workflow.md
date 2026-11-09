# Library reopen shortcut workflow

Ctrl+Alt+Shift+P now routes through the shared selected-track reopen lifecycle
instead of opening the source directly. This preserves stale-visualization
reset, recovery announcements, and keyboard focus consistently with the
visible Reopen selected control.

The workflow is covered by
`tests/unit/check_issue76_library_reopen_shortcut_contract.py`.
