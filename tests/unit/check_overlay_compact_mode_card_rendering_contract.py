from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-compact-mode-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Choose a compact mode", "Compact", "Medium", "Large", "Preview updates instantly", "current position")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing compact mode rendering copy: {missing}")
if "RECOMMENDED" not in text:
    raise SystemExit("compact mode card must identify the recommended option")
print("overlay compact mode card rendering contract passed")
