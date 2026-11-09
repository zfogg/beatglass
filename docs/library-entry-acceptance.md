# Library entry activation acceptance

`Ctrl+Alt+Enter` activates the focused entry in the Library browser. It is
enabled only for a focused entry in the current-folder browser (not recursive
results), then uses the same folder-open or audio-selection path as pointer
activation. Folder activation resets the prior selection and filter; audio
activation inspects the metadata and moves focus to analysis.

Portable coverage lives in
`tests/unit/check_issue76_library_activate_entry_shortcut_contract.py`.
