# Library recursive-results Escape reset

Escape from recursive results now returns to the folder browser with the
recursive index, active filter, and browser entry selection cleared. The
announcement describes the reset so the resulting browsing context is clear.
The Escape event is consumed after the reset so it cannot bubble into another
control.

Portable coverage lives in
`tests/unit/check_issue76_library_recursive_escape_reset_contract.py`.
