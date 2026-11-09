from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-failed-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Could not send feedback", "Feedback was not sent", "Nothing was saved remotely", "remain available locally", "Try again later", "Close this message", "Close", "Retry")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing failed feedback rendering copy: {missing}")
if "#d96d70" not in text or "#56d38b" not in text:
    raise SystemExit("failed feedback card must distinguish error and close actions")
print("overlay feedback failed card rendering contract passed")
