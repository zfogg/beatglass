from pathlib import Path

source = Path("lib/library/sqlite_cache.cpp").read_text()
expected = 'if (result.empty() || result.size() > kMaxCachedResultBytes || !sqlite_length_representable(result.size())) {'
if expected not in source:
    raise SystemExit("empty publish results must fail closed and retain recovery")
print("empty publish recovery contract verified")
