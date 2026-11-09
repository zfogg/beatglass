# Library clear-selection behavior

The visible Clear selection action is enabled only when a source is selected.
Activating it clears source and analysis context, announces the empty state,
and preserves focus on the action so keyboard users can review the reset.
The shared clear-selection function owns the announcement so pointer and
keyboard paths do not produce duplicate status messages.

Portable coverage lives in
`tests/unit/check_issue76_library_clear_selection_contract.py`.
