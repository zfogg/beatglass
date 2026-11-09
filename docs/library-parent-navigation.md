# Library parent navigation

`Ctrl+Alt+Up` navigates from the current Library folder to its parent when a
parent exists. It uses the same reset semantics as the visible Up button:
selection, filtering, and recursive results are cleared, then focus returns
to the folder browser with an announcement describing the new context.

Portable coverage lives in
`tests/unit/check_issue76_library_parent_shortcut_contract.py`.
