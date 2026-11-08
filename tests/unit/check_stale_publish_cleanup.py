from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/cache_identity.cpp").read_text(encoding="utf-8")
required = ["initial == FingerprintStatus::changed", "cache.invalidate(key)", "return Publish::stale_input"]
missing = [m for m in required if m not in source]
if missing: raise SystemExit("Missing stale publication cleanup markers: " + ", ".join(missing))
if source.index("cache.invalidate(key)") > source.index("cache.begin(run_id, key)"):
    raise SystemExit("stale cleanup must precede retryable begin")
print(f"Stale publication cleanup contract verified ({len(required)} checks)")
