# Library folder chooser cancellation

Canceling the folder picker now uses a named recovery action. It keeps the
current folder open, announces that no change was made, and restores focus to
Choose folder for an immediate retry.

The contract is covered by
`tests/unit/check_issue76_library_folder_chooser_cancel_contract.py`.
