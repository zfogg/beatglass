from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/cache_identity.cpp").read_text(encoding="utf-8")
required = ["if (stop.stop_requested()) return false;", "if (status == FingerprintStatus::changed) cache.invalidate(key);", "cache.publish"]
missing = [m for m in required if m not in source]
if missing: raise SystemExit("Missing cancellation stale cleanup markers: " + ", ".join(missing))
callback = source[source.index("cache.publish(run_id"):]
if "if (stop.stop_requested()) { cache.invalidate(key);" in callback:
    raise SystemExit("Cancellation callback must preserve the prior cache row")
print(f"Cancellation stale cleanup contract verified ({len(required)} checks)")
