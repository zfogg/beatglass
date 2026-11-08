from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/cache_identity.cpp").read_text(encoding="utf-8")
header = (root / "include/beatglass/library/detail/cache_identity.hpp").read_text(encoding="utf-8")
required = ["content identity changed; catalog entry removed", "source path missing; catalog entry removed", "changed content removes it"]
missing = [m for m in required if m not in source + header]
if missing: raise SystemExit("Missing catalog diagnostic markers: " + ", ".join(missing))
print(f"Catalog diagnostic contract verified ({len(required)} checks)")
