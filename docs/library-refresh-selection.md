# Library refresh selection behavior

Refreshing the current folder re-reads the catalog and deliberately clears
the active entry and selected source. This prevents playback or analysis from
continuing to refer to a file that was removed or changed while the folder was
stale. The Refresh action now exposes that consequence in its accessible
value, and restores focus to the browser after the refresh.

The behavior is covered by
`tests/unit/check_issue76_library_refresh_selection_contract.py`.
