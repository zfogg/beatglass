# Recursive analysis stale-evidence reset

When recursive analysis accepts one or more audio files, the Library clears
the prior visualization evidence before queueing fresh work. This prevents a
previous source's visualization from being presented as the result of the new
recursive batch. A zero-acceptance submission leaves the current evidence
untouched and still reports the queue outcome.

The behavior is covered by
`tests/unit/check_issue76_library_recursive_analysis_stale_reset_contract.py`.
