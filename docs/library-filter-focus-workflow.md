# Library filter focus workflow

Ctrl+L now uses a named Library filter-focus action. It focuses the filter,
selects its current text for replacement, and announces the state. Keeping
this in one action makes the keyboard entry path explicit and reusable by
future browser controls.

The workflow is covered by
`tests/unit/check_issue76_library_filter_focus_workflow_contract.py`.
