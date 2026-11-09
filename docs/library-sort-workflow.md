# Library sort workflow

The sort button and Ctrl+Alt+S shortcut now share one action. Both reverse
name ordering, announce the resulting ascending/descending state, and restore
focus to the sort control. This keeps pointer and keyboard sorting behavior
consistent for the current folder browser.

The workflow is covered by
`tests/unit/check_issue76_library_sort_workflow_contract.py`.
