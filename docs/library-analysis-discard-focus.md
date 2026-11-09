# Library analysis-recovery discard focus

The visible Discard recovery action now announces the confirmation context,
focuses Cancel when the dialog opens, and restores focus to Discard recovery
when canceled. The dialog continues to state that source audio is unchanged.

Portable coverage lives in
`tests/unit/check_issue76_library_analysis_discard_focus_contract.py`.
