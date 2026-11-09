# Library cache mode validation

Cache actions now share one readiness contract. Content, engine, and
configuration identities must be present, and mode must be exactly `offline`
or `live`. Check cache, Forget cached result, and Run fresh analysis all use
that same gate, preventing an ambiguous mode from reaching the storage
boundary. The source audio remains untouched; fresh analysis still requires a
selected local source and confirmation.

The contract is covered by
`tests/unit/check_issue76_library_cache_mode_validation_contract.py`.
