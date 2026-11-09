# Library browser selection reset

Folder refresh uses the shared `resetLibraryBrowserSelection` workflow. It
clears the selected source, clears the browser row selection, announces the
transition, and restores keyboard focus to the browser. Keeping those actions
together prevents a refreshed folder from leaving playback or analysis
controls pointed at an entry that is no longer current.

The workflow contract is covered by
`tests/unit/check_issue76_library_browser_selection_reset_contract.py`.
