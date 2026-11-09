# Library analysis submission errors

The desktop bridge treats operation construction and admission as a fallible
boundary. Invalid identities, unavailable storage, and decoder setup errors
are caught on the worker thread before a handle is exposed to the UI. The
bridge clears any previous handle and emits `Analysis submission rejected:`
plus the stable `Analysis rejected` state, so a failed submission cannot leave
the previous job appearing active.

This is distinct from worker-time failure after admission, which remains
retryable through the existing job lifecycle and recovery controls.
