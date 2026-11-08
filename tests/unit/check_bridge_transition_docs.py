from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["bridge transition", "queued/running/completed/cancelled/failed", "endpoint loss", "reacquisition", "check_desktop_analysis_transition.py", "native desktop/audio/overlay"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing bridge transition markers: " + ", ".join(missing))
print(f"Bridge transition documentation verified ({len(required)} checks)")
