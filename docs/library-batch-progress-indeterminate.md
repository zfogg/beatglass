# Library indeterminate batch progress

When a batch has started but no item has completed yet, the progress accessibility
value now says “in progress” before its counts. This avoids presenting a zero
percent value as if work were idle while preserving the detailed counters.
