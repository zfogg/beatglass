# Library recursive selection reset

Leaving recursive results through Escape, the Return button, or the immediate
return shortcut now clears the selected recursive source as well as the
indices and filter. This prevents stale recursive metadata or analysis actions
from remaining active in the folder browser.

Portable coverage lives in
`tests/unit/check_issue76_library_recursive_selection_reset_contract.py`.
