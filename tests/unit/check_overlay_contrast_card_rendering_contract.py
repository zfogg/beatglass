from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-contrast-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Contrast preview", "Midnight", "High contrast", "Large labels", "Healthy / ready", "Needs attention", "Neutral / paused", "Keep contrast")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing contrast rendering copy: {missing}")
for color in ("#56d38b", "#e3b45b", "#718096"):
    if color not in text:
        raise SystemExit(f"contrast card missing state color {color}")
print("overlay contrast card rendering contract passed")
