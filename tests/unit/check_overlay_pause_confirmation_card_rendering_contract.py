from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-pause-confirmation-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Pause monitoring?", "No audio will be captured", "Position, size, and opacity", "You can resume any time", "Ctrl + Shift + P", "Pause now", "Keep listening")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing pause confirmation rendering copy: {missing}")
if "#e3b45b" not in text or "#2a3444" not in text:
    raise SystemExit("pause confirmation must distinguish warning and cancel actions")
print("overlay pause confirmation card rendering contract passed")
