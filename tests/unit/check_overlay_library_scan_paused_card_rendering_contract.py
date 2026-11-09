from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-scan-paused-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Scan paused", "progress is saved", "128 of 480 tracks", "0 files changed", "Resume whenever", "overlay stays live", "Resume scan", "Close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing paused-scan rendering copy: {missing}")
if "#e3b45b" not in text or "#56d38b" not in text:
    raise SystemExit("paused scan card must distinguish paused state and resume action")
print("overlay library scan paused card rendering contract passed")
