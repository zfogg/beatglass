from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-feedback-privacy-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Before you send", "Your audio stays private", "Only your rating", "No audio files or library data", "You choose what to share", "Feedback is optional", "Continue", "Skip")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing feedback privacy rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("feedback privacy card must distinguish continue and skip actions")
print("overlay feedback privacy card rendering contract passed")
