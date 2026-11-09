# Library storage recovery health

The Library surface now reports storage recovery health beside the catalog. It
shows the current storage transition and count of saved retryable runs, and
directs users to reopen storage when it is closed or errored. The status is
read-only; retry and discard remain guarded by the persisted recovery owner.
