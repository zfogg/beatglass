# Library browser parent navigation

Backspace parent navigation now matches the visible Up action: it clears the
selected source, filter, and recursive-results state before moving to the
parent folder, resets the browser index, and consumes the key event.

Portable coverage lives in
`tests/unit/check_issue76_library_browser_parent_contract.py`.
