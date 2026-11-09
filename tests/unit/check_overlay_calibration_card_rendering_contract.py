from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-calibration-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Tune your view", "LIVE PREVIEW", "Top right", "Compact", "75%", "Keep this view", "Changes are reversible")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing calibration rendering copy: {missing}")
if "Previewing" not in text or "#56d38b" not in text:
    raise SystemExit("calibration card must show live preview and active action")
print("overlay calibration card rendering contract passed")
