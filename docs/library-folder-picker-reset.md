# Library folder picker reset

Accepting a new folder uses the same browser selection reset as refresh. The
current source and row selection are cleared, the recursive result view is
closed, the filter is reset, and focus returns to the browser. This keeps a
folder change from carrying a stale source or recursive result into the new
location.

The contract is covered by
`tests/unit/check_issue76_library_folder_picker_reset_contract.py`.
