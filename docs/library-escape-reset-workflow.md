# Library Escape reset workflow

Escape from the folder browser now uses one action to clear the selected
source, filter text, and active row before returning focus to the filter. This
provides a predictable keyboard reset without leaving playback or analysis
controls pointed at the previous entry.

The workflow is covered by
`tests/unit/check_issue76_library_escape_reset_contract.py`.
