# Library playback action accessibility

The Library playback controls expose their readiness and target through the
accessible value as well as the label and description:

- **Open** and **Play selected** report `available; <track name>` when a
  selected source and playback bridge are present.
- They report an actionable unavailable state when the user must select a
  source, or when playback is unavailable in the current build.
- **Reopen selected** reports the replacement target, so a screen reader user
  can distinguish it from opening the current source.

The controls retain their existing keyboard focus behavior. Clicking either
action routes through the same playback lifecycle and stale-visualization
reset used by the visual UI; the value is status context, not a second source
of truth. The contract is checked by
`tests/unit/check_issue76_library_playback_action_accessible_state_contract.py`.
