# Library folder chooser focus

The folder-chooser focus shortcut now uses a named action that selects the
Library page, restores focus to the chooser, and announces the target. This
makes keyboard navigation consistent with the other Library focus workflows.

The contract is covered by
`tests/unit/check_issue76_library_folder_chooser_focus_contract.py`.
