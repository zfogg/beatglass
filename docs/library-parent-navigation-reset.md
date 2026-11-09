# Library parent navigation reset

Moving to a parent folder now uses the shared browser reset workflow after
updating the folder path. The selected source, row selection, filter, and
recursive results are cleared together, and focus returns to the browser so
the new folder is immediately navigable.

The behavior is covered by
`tests/unit/check_issue76_library_parent_navigation_reset_contract.py`.
