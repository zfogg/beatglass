# Library sort-order shortcut

The Library exposes a global `Ctrl+Alt+S` shortcut for reversing the file-name
sort order. The action updates the same `sortDescending` property as the
visible sort button, announces the resulting ascending or descending order,
and returns focus to the button so keyboard users can confirm the state.

Portable coverage lives in
`tests/unit/check_issue76_library_sort_shortcut_contract.py`.

The Library also keeps a live accessible selection status directly above its
controls. It reports whether a source is selected and names the source when
one is active, so playback and analysis actions have an explicit target.
`Ctrl+Alt+Backspace` clears that selection, announces the reset, and returns
focus to the browser.
