# Library browser Escape recovery

Escape from the Library browser now clears both the selected source and active
filter, resets the browser index, returns focus to the filter, and announces
that all folder entries are visible. This gives one predictable recovery path
from keyboard browsing.
The Escape event is consumed after this reset so it cannot activate another
control.

Portable coverage lives in
`tests/unit/check_issue76_library_browser_escape_contract.py`.
