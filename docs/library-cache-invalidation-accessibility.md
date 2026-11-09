# Library cache invalidation accessibility state

Forget cached result reports when it is available because a completed cache hit
has been confirmed, and when it is unavailable because Check cache must run first.
This makes the destructive cache action's prerequisite clear to assistive
technology before the confirmation dialog is opened.
