# Library cache mode input

The cache identity mode field now validates its input against the storage
contract (`offline` or `live`). An invalid value is announced as invalid and
cannot satisfy the shared cache-action readiness gate. This keeps lookup,
invalidation, and reanalysis from sending undocumented modes to persistence,
while preserving keyboard editing and the existing source-audio safety rules.

The regression contract is covered by
`tests/unit/check_issue76_library_cache_mode_input_contract.py`.
