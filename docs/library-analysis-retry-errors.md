# Library analysis retry errors

Retry is a lifecycle boundary, not just a button state. The worker bridge now
catches retry failures, clears a handle that can no longer be retried, and
reports `Analysis retry rejected:` rather than leaving a stale retryable job
visible. A retry that is accepted continues through the existing queued and
progress states; a late or invalid retry cannot resurrect completed or
discarded work.
