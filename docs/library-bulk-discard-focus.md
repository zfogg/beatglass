# Library bulk recovery discard

The bulk discard action now announces its confirmation scope before opening the
dialog: persisted recovery records are removed, while source audio remains
unchanged. The existing dialog keeps Cancel as the initial focus and returns
focus to the bulk action after cancellation or completion, so keyboard users can
review the destructive scope before proceeding.
