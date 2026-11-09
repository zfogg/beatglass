# Library parent navigation workflow

The Up button and Ctrl+Alt+Up shortcut now share one parent-navigation action.
It updates the folder, clears the filter and recursive results, resets the
selected source, and restores browser focus through the existing reset path.

The workflow is covered by
`tests/unit/check_issue76_library_parent_navigation_workflow_contract.py`.
