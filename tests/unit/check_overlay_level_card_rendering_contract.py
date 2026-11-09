from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-level-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Audio level", "CURRENT LEVEL", "−12 dB", "Healthy headroom", "Signal guidance", "Comfortable", "Watch", "Clipping risk", "Updated live")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing level rendering copy: {missing}")
for color in ("#56d38b", "#e3b45b", "#d96d70"):
    if color not in text:
        raise SystemExit(f"level card missing guidance color {color}")
print("overlay level card rendering contract passed")
