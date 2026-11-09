from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-retry-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Sending feedback", "Keep working", "Sending securely", "Rating and optional note", "Audio files and library data", "Monitoring is unaffected", "overlay stays live", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing retry feedback rendering copy: {missing}")
if "stroke-dasharray" not in text or "#e3b45b" not in text:
    raise SystemExit("retry feedback card must show an in-progress state")
print("overlay feedback retry card rendering contract passed")
