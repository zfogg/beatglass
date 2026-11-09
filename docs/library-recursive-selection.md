# Library recursive selection

Selecting a recursive result now synchronizes the list's current index before
updating the selected source. Metadata inspection, stale-analysis reset,
announcement, and focus on the analysis action remain part of the same
selection transaction.

Portable coverage lives in
`tests/unit/check_issue76_library_recursive_selection_contract.py`.
