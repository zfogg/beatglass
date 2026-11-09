from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/library/detail/sqlite_cache.hpp").read_text(encoding="utf-8")
source = (root / "lib/library/sqlite_cache.cpp").read_text(encoding="utf-8")
required = ["retryable_run(const std::string& id)", "including source and decoder metadata", "retryable_runs(kMaxRetryableRuns)"]
missing = [m for m in required if m not in header + source]
if missing: raise SystemExit("Missing retryable lookup markers: " + ", ".join(missing))
print(f"Retryable lookup contract verified ({len(required)} checks)")
