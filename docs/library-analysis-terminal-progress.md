# Library terminal progress lifecycle

Queued and running jobs report source-frame progress. When a job is cancelled
or fails, the bridge now resets that progress to zero while retaining the
retryable terminal state. A later retry therefore starts with a clean progress
display instead of inheriting frames from the abandoned attempt.
