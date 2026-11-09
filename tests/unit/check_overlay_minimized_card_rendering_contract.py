from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-minimized-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Overlay minimized", "Live monitoring is on", "Built-in Speakers", "0 dropped frames", "Bring it back", "Show overlay", "Open controls", "Ctrl + Shift + H")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing minimized rendering copy: {missing}")
if "position, size, and opacity" not in text:
    raise SystemExit("minimized card must communicate retained layout settings")
print("overlay minimized card rendering contract passed")
