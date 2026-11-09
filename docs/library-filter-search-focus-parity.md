# Library filter search-focus parity

Ctrl+F and Ctrl+L now share the same filter-focus implementation. Ctrl+F also
selects the Library navigation page before focusing the filter, while both
shortcuts select existing text and announce the appropriate next action.

The shared behavior is covered by
`tests/unit/check_issue76_library_filter_focus_workflow_contract.py`.
