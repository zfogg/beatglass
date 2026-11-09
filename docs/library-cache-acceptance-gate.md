# Library cache acceptance gate

Issue #1 requires completed analysis to be reused only when the validated
content identity and analysis configuration still match. The acceptance case
in `tests/unit/analysis_operation_test.cpp` now exercises that boundary:

- a completed run for `sha256:file-v1`/`config-a` is reused without reserving
  another analysis run;
- changing the content identity produces a cache miss and reserves fresh work;
- changing the analysis configuration also produces a miss and reserves fresh
  work.

This is a deterministic storage/admission acceptance scenario. It does not
claim that a hash is computed by the UI; production callers remain responsible
for obtaining and revalidating the content identity before admission.
