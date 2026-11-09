from pathlib import Path

source = Path("include/beatglass/library/detail/sqlite_cache.hpp").read_text()
for phrase in ("Empty or oversized results fail closed", "leave a running row retryable", "stale verification"):
    if phrase not in source:
        raise SystemExit(f"missing publish contract documentation: {phrase}")
print("sqlite publish lifecycle contract verified")
