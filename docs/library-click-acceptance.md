# Library single-click acceptance

Single-click activation in both the folder browser and recursive results now
consumes the pointer event after updating selection. This prevents a click
from bubbling into surrounding controls while preserving metadata and
analysis focus behavior.

Portable coverage lives in
`tests/unit/check_issue76_library_click_acceptance_contract.py`.
