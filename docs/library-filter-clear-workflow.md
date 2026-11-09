# Library filter-clear workflow

The clear-filter shortcut and Escape key now share one action. Both clear the
current folder filter, refresh the catalog through the existing text binding,
announce that all entries are visible, and return focus to browser results.

The workflow is covered by
`tests/unit/check_issue76_library_filter_clear_workflow_contract.py`.
