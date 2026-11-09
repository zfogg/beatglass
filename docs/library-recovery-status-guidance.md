# Library recovery status guidance

Recovery status now directs users to review admission and queue status before
choosing Retry or discard recovery. This keeps persisted recovery decisions tied
to the current lifecycle state and avoids treating an unqualified reset as a
ready-to-retry result.
