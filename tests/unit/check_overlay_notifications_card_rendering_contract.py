from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-notifications-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Recent activity", "Overlay restored", "Output changed", "Monitoring paused", "Built-in Speakers", "Clear activity", "3 events")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing notifications rendering copy: {missing}")
if text.count("<circle") < 3:
    raise SystemExit("notifications card must distinguish each event visually")
print("overlay notifications card rendering contract passed")
