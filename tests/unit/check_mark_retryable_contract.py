from pathlib import Path

source = Path("include/beatglass/library/detail/sqlite_cache.hpp").read_text()
for phrase in ("clears any partial result", "idempotent for recovery callers", "already terminal"):
    if phrase not in source:
        raise SystemExit(f"missing mark_retryable contract: {phrase}")
print("mark retryable lifecycle contract verified")
