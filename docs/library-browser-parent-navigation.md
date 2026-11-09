# Library browser parent navigation

Backspace parent navigation now matches the visible Up action: it clears the
selected source, filter, and recursive-results state before moving to the
parent folder, resets the browser index, and consumes the key event.

At the filesystem root, Backspace is still consumed and announces that no
parent is available instead of silently doing nothing.

Portable coverage lives in
`tests/unit/check_issue76_library_browser_parent_contract.py`.
