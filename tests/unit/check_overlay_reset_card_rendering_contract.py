from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-reset-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Reset settings?", "What will reset", "Position · size · opacity", "audio files and library", "Recommended defaults", "Reset settings", "Keep settings", "Changes are reversible")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing reset rendering copy: {missing}")
if "#d96d70" not in text or "#2a3444" not in text:
    raise SystemExit("reset card must distinguish destructive and safe actions")
print("overlay reset card rendering contract passed")
