from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-platform-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Platform support", "WINDOWS", "MACOS", "LINUX", "Ready", "Planned", "Qualification in progress", "View evidence")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing platform rendering copy: {missing}")
if text.count("✓") != 2 or "#e3b45b" not in text:
    raise SystemExit("platform card must distinguish ready and planned support")
print("overlay platform card rendering contract passed")
