from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-status-bar-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("At-a-glance status", "Listening", "Built-in Speakers", "48 kHz", "Healthy", "Capture state", "Selected output", "Customize bar", "Updates live")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing status-bar rendering copy: {missing}")
if text.count("●") != 4:
    raise SystemExit("status bar card must enumerate four visible indicators")
print("overlay status bar card rendering contract passed")
