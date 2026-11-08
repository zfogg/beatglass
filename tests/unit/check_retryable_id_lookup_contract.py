from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/sqlite_cache.cpp").read_text(encoding="utf-8")
header = (root / "include/beatglass/library/detail/sqlite_cache.hpp").read_text(encoding="utf-8")
required = [
    'WHERE id=? AND status=\'retryable\'',
    'SELECT id,content,engine,config,mode,source_path,decoder_snapshot',
    'invalid run ID',
    'independent of the bounded list page',
]
missing = [marker for marker in required if marker not in source + header]
if missing:
    raise SystemExit("Missing interrupted-recovery lookup markers: " + ", ".join(missing))
method = source[source.index("std::optional<RetryableRun> retryable_run") : source.index("bool discard_retryable", source.index("std::optional<RetryableRun> retryable_run"))]
if "retryable_runs(" in method:
    raise SystemExit("ID lookup must not scan the bounded retryable list")
print(f"Interrupted recovery ID lookup contract verified ({len(required)} checks)")
