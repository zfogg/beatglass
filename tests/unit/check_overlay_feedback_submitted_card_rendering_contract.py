from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-submitted-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Thanks for the note", "Feedback sent", "Rating: 4 / 5", "No further action is needed", "Back to your work", "overlay is ready", "Close", "Esc to close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing submitted feedback rendering copy: {missing}")
if "#56d38b" not in text:
    raise SystemExit("submitted feedback card must show success")
print("overlay feedback submitted card rendering contract passed")
