from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = "\n".join((root / p).read_text(encoding="utf-8") for p in ("README.md", "docs/progress.md", "docs/platform-qualification-status.md"))
required = ["decoder_snapshot", "compatibility", "legacy", "rejected", "fail-closed"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing decoder compatibility markers: " + ", ".join(missing))
print(f"Decoder compatibility documentation contract verified ({len(required)} checks)")
