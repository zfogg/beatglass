from pathlib import Path

source = Path("lib/library/sqlite_cache.cpp").read_text()
expected = 'if (result.size() > kMaxCachedResultBytes || !sqlite_length_representable(result.size())) {\n        if (!retryable()) { rollback(); return Publish::error; }'
if expected not in source:
    raise SystemExit("oversized publish results must transition running rows to retryable")
print("oversized publish recovery contract verified")
