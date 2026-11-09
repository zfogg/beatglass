# Library bulk discard accessibility state

Discard all saved runs exposes the count of retryable persisted runs when it is
available. When disabled, assistive technology receives an explicit no-retryable-
runs value instead of an empty control state, while the existing confirmation and
source-audio safety copy remains in place.
