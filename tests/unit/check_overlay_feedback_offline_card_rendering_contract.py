from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-offline-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Feedback is offline", "Nothing was sent", "could not reach", "remain on this device", "What you can do", "Try again later", "Try again", "Close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing offline feedback rendering copy: {missing}")
if "#718096" not in text or "#56d38b" not in text:
    raise SystemExit("offline feedback card must show neutral state and retry action")
print("overlay feedback offline card rendering contract passed")
