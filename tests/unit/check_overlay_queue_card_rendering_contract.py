from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-queue-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Up next", "CURRENT", "Midnight Drive", "playing now", "NEXT", "Afterglow", "queued", "Open library", "Queue stays local")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing queue rendering copy: {missing}")
if "01:42" not in text or "03:58" not in text:
    raise SystemExit("queue card must expose current and next durations")
print("overlay queue card rendering contract passed")
