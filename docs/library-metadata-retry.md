# Library metadata retry

When metadata inspection fails for the selected source, the visible Retry
metadata action remains available and keyboard reachable. Activating it
re-inspects the selected URL, announces the retry request, and restores focus
to the retry action so the updated status can be reviewed.

Portable coverage lives in
`tests/unit/check_issue76_library_metadata_retry_contract.py`.
