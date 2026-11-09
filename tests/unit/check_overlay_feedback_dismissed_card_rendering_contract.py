from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-dismissed-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("No problem", "Feedback skipped", "Nothing was sent", "monitoring session is unchanged", "will not interrupt", "Back to your work", "Close", "Give feedback later")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing dismissed feedback rendering copy: {missing}")
if "#718096" not in text or "#56d38b" not in text:
    raise SystemExit("dismissed feedback card must show neutral state and return action")
print("overlay feedback dismissed card rendering contract passed")
