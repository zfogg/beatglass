# Library recursive-action focus

The recursive-catalog focus shortcut now uses a named action that selects the
Library page, focuses Add folder recursively, and announces the target. Its
existing folder-present and non-recursive guards remain unchanged.

The contract is covered by
`tests/unit/check_issue76_library_recursive_action_focus_contract.py`.
