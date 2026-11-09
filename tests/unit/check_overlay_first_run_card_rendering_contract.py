from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-first-run-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Set up your overlay", "Choose an output", "Choose a size", "Start monitoring", "Built-in Speakers", "Compact", "Controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing first-run rendering copy: {missing}")
if text.count('fill="#56d38b"') < 3:
    raise SystemExit("first-run card must show progressive completion accents")
print("overlay first-run card rendering contract passed")
