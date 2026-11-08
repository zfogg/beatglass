from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["decoder snapshot", "codec feature configuration", "fixture hash", "exact command", "controlled failure", "preparation evidence"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing native codec evidence markers: " + ", ".join(missing))
print(f"Native codec evidence documentation verified ({len(required)} checks)")
