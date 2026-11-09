from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-output-picker-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Select an output", "Built-in Speakers", "USB Headphones", "HDMI Display", "Selected", "Unavailable", "Save output", "remembered")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing output picker rendering copy: {missing}")
if "#56d38b" not in text or "#e3b45b" not in text:
    raise SystemExit("output picker must distinguish selected and unavailable states")
print("overlay output picker card rendering contract passed")
