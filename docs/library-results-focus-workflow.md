# Library results focus workflow

The browser and results shortcuts now share one focus action. It selects the
Library page and focuses recursive results when recursive browsing is active,
otherwise focusing the current-folder browser. The announcement identifies
which result surface received focus.

The workflow is covered by
`tests/unit/check_issue76_library_results_focus_workflow_contract.py`.
