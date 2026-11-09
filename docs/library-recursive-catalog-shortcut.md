# Library recursive-catalog shortcut

Ctrl+Alt+R now routes through a named recursive-catalog request action. It
invokes the same Add folder recursively control as the visible UI. The button
owns the single catalog-result announcement, avoiding duplicate announcements
for keyboard activation.

The workflow is covered by
`tests/unit/check_issue76_library_recursive_catalog_shortcut_contract.py`.
