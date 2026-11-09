# Library navigation acceptance

The Library now exposes its keyboard workflow in the UI. Shortcut help lists
filter, browser, results, sort, and selection-clear actions in one accessible
status surface, including recursive-catalog and parent-folder navigation.
The sort-control focus shortcut (`Ctrl+Alt+Shift+O`) gives keyboard users a
stable entry point before reversing the order with `Ctrl+Alt+S`.
Activating the Library tab also announces the destination, which makes global
navigation state observable for keyboard and assistive technology users.

Portable coverage lives in
`tests/unit/check_issue76_library_shortcut_help_contract.py`.
