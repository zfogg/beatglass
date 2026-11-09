# Library refresh keyboard parity

The `Ctrl+R` Library refresh shortcut now uses the same reset workflow as the
Refresh button: refresh the catalog, clear the selected source and browser
row, announce the transition, and restore browser focus. This keeps keyboard
and pointer refresh behavior equivalent.

The parity contract is covered by
`tests/unit/check_issue76_library_refresh_keyboard_parity_contract.py`.
