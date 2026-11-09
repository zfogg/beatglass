from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/cache_identity.cpp").read_text(encoding="utf-8")
required = ["if (stop.stop_requested()) return Publish::cancelled;", "cache.begin(run_id, key);", "result.empty()"]
missing = [m for m in required if m not in source]
if missing: raise SystemExit("Missing cancellation publication markers: " + ", ".join(missing))
if source.index("stop.stop_requested()) return Publish::cancelled") > source.index("cache.begin(run_id, key)"):
    raise SystemExit("cancellation guard must precede cache begin")
print("Cancellation publication contract verified (3 checks)")
