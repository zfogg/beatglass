# Library batch recovery banner

The Library surface now exposes a compact batch status banner alongside the
catalog result count. It reports accepted, completed, failed, and cancelled
work while a batch is retained, including a clear recovery cue for terminal
states. The existing selected-source controls remain the single action owner:
retry clears stale presentation before admission and discard requires
confirmation. This keeps recovery discoverable from the full Library screen
without duplicating worker operations.
