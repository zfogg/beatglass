# Library folder-picker cancellation

Canceling the native Library folder picker now announces that the current
folder remains open and restores focus to Choose folder. Accepting a folder
continues to clear stale selection and recursive state before focusing the
browser.

Portable coverage lives in
`tests/unit/check_issue76_library_folder_picker_reject_contract.py`.
