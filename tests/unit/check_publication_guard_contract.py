from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/cache_identity.cpp").read_text(encoding="utf-8")
required = ["if (result.empty()) return Publish::error;", "revalidate_file_identity(path, key.content, stop)", "stale_input"]
missing = [m for m in required if m not in source]
if missing: raise SystemExit("Missing publication guard markers: " + ", ".join(missing))
print(f"Publication guard contract verified ({len(required)} checks)")
