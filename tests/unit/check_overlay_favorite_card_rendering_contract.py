from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-favorite-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Pinned views", "Compact · Built-in Speakers", "Medium · USB Headphones", "75% opacity", "85% opacity", "Pinned locally", "Use selected", "Edit pins in controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing favorites rendering copy: {missing}")
if text.count("★") != 2:
    raise SystemExit("favorites card must show both pinned views")
print("overlay favorite card rendering contract passed")
