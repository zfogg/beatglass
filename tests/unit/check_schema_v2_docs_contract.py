from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = "\n".join((root / p).read_text(encoding="utf-8") for p in ("README.md", "docs/progress.md", "docs/platform-qualification-status.md", "include/beatglass/library/detail/sqlite_cache.hpp"))
required = ["schema v2", "source_path", "decoder_snapshot", "metadata-gated", "native qualification"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing schema v2 documentation markers: " + ", ".join(missing))
print(f"Schema v2 documentation contract verified ({len(required)} checks)")
