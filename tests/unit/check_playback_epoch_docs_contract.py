from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["epoch change", "stale queued evidence", "reacquisition", "native device-loss", "output loss"]
missing = [marker for marker in required if marker not in text]
if missing: raise SystemExit("Missing playback epoch documentation markers: " + ", ".join(missing))
print(f"Playback epoch documentation contract verified ({len(required)} checks)")
