from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-no-signal-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Waiting for signal", "No active audio yet", "Built-in Speakers", "Overlay remains visible", "Play audio, then watch", "Open controls", "expected state")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing no-signal rendering copy: {missing}")
if "#718096" not in text or "#56d38b" not in text:
    raise SystemExit("no-signal card must distinguish neutral state from action")
print("overlay no-signal card rendering contract passed")
