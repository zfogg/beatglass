# Library analysis retry focus

The visible Retry analysis action now preserves focus after dispatching the
shared analysis retry path. That path owns the success/failure announcement,
so retry recovery presents one status update while keeping the action
keyboard reachable.

Portable coverage lives in
`tests/unit/check_issue76_library_analysis_retry_focus_contract.py`.
