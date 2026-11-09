from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-history-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Listening history", "private, local record", "Midnight Drive", "Afterglow", "Soft Focus", "Clear history", "Stored locally")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing history rendering copy: {missing}")
for label in ("JUST NOW", "12 MIN AGO", "YESTERDAY"):
    if label not in text:
        raise SystemExit(f"history card missing time grouping {label}")
print("overlay history card rendering contract passed")
