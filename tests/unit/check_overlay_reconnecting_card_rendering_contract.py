from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-reconnecting-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Reconnecting", "lost the selected output", "Retrying automatically", "No audio is being captured", "What stays safe", "Position, size, and opacity", "Retry now", "Open controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing reconnecting rendering copy: {missing}")
if text.count("✓") != 2:
    raise SystemExit("reconnecting card must show retained settings safeguards")
print("overlay reconnecting card rendering contract passed")
