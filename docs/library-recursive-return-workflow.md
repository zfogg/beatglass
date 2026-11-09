# Library recursive-results return workflow

The Return to folder browser action and Escape key now share one workflow. It
closes recursive results, clears their active row and filter, resets the
selected source, and restores focus to the folder browser. This keeps mouse,
keyboard, and accessible navigation paths consistent after recursive browsing.

The workflow is covered by
`tests/unit/check_issue76_library_recursive_return_workflow_contract.py`.
