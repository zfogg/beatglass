# Library analysis-cache identity accessibility

The cache lookup form now names each identity field for assistive technology:
content identity, engine, configuration, and mode. Their descriptions explain
that the values identify a completed analysis cache entry and do not modify
source audio. The existing Check cache, Forget cached result, and Run fresh
analysis actions remain gated on complete identity fields and preserve their
existing confirmation and keyboard-focus behavior.

The contract is covered by
`tests/unit/check_issue76_library_cache_identity_accessibility_contract.py`.
