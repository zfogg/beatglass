# Library browser key acceptance

Enter, Return, and Space activation in the Library browser now explicitly
consume their key events after activating the focused entry. This prevents
the same key from bubbling into another control while preserving folder-open
and audio-selection behavior.

Portable coverage lives in
`tests/unit/check_issue76_library_browser_key_acceptance_contract.py`.
