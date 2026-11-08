from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["Shortcut-triggered stop/resume", "accessible announcement", "preserve focus", "screen-reader", "check_desktop_analysis_transition.py"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing shortcut announcement markers: " + ", ".join(missing))
print(f"Shortcut announcement documentation verified ({len(required)} checks)")
