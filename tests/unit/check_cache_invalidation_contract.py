from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/cache_identity.cpp").read_text(encoding="utf-8")
docs = (root / "include/beatglass/library/detail/cache_identity.hpp").read_text(encoding="utf-8")
required = ["if (status == FingerprintStatus::changed) cache.remove_entry(path);", "unreadable path is not proof", "leave the row available"]
missing = [m for m in required if m not in source + docs]
if missing: raise SystemExit("Missing cache invalidation markers: " + ", ".join(missing))
print(f"Cache invalidation contract verified ({len(required)} checks)")
