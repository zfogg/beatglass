from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["Keyboard playback recovery parity", "focus order", "accessible names", "pointer input", "check_desktop_analysis_transition.py"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing keyboard recovery markers: " + ", ".join(missing))
print(f"Keyboard recovery documentation verified ({len(required)} checks)")
