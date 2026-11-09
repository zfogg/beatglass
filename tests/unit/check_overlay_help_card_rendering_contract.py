from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-help-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Need a hand?", "Keyboard shortcuts", "Troubleshooting", "Open diagnostics", "ABOUT BEATGLASS", "Local processing", "Close help", "Esc to close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing help rendering copy: {missing}")
if text.count("#56d38b") < 3:
    raise SystemExit("help card must expose multiple actionable destinations")
print("overlay help card rendering contract passed")
