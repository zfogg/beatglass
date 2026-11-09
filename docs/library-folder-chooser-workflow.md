# Library folder chooser workflow

The Choose folder button now uses a named action that opens the folder picker,
announces the dialog, and restores focus to the chooser control. This keeps the
visible folder-selection path explicit and reusable by keyboard workflows.

The contract is covered by
`tests/unit/check_issue76_library_folder_chooser_workflow_contract.py`.
