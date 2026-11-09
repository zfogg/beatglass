from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("How is it going?", "How does the overlay feel?", "Choose one", "Optional note", "What should we improve?", "Send feedback", "Not now", "4 / 5")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing feedback rendering copy: {missing}")
if text.count("★") != 5:
    raise SystemExit("feedback card must show a five-point rating control")
print("overlay feedback card rendering contract passed")
