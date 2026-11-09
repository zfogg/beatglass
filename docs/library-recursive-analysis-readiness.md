# Library recursive analysis readiness

Recursive analysis is enabled only when the recursive catalog contains at
least one audio result and no analysis batch is active. The action announces
the number of available audio results, rather than treating folders-only
results as analyzable work. It still submits the bounded first-file batch and
returns focus to the recursive results view.

The contract is covered by
`tests/unit/check_issue76_library_recursive_audio_count_contract.py`.
