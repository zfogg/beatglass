# Persisted recovery selection status

The Library now reports the selected persisted recovery run and whether retry
is currently available directly beside the saved-run selector. The selector
still owns run admission, while retry and discard remain separate actions; the
status text makes the current selection and terminal-state eligibility visible
to keyboard and assistive-technology users without modifying source audio.
