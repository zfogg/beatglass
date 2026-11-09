from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-delete-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Remove pinned view?", "Compact speakers", "saved view will no longer", "Still available", "recreate this setup", "Remove pin", "Keep pin")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing delete rendering copy: {missing}")
if "#d96d70" not in text or "#2a3444" not in text:
    raise SystemExit("delete card must distinguish remove and keep actions")
print("overlay delete card rendering contract passed")
