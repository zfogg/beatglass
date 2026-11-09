# Library analyze shortcut workflow

Ctrl+Alt+A now routes through a shared selected-source analysis action. An
accepted submission clears stale visualization evidence before fresh analysis,
announces the queue outcome, and restores focus to Analyze selected, matching
the visible control's lifecycle.

The workflow is covered by
`tests/unit/check_issue76_library_analyze_shortcut_contract.py`.
