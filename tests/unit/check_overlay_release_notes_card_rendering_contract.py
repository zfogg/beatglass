from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-release-notes-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("What changed", "BEATGLASS 0.2", "More reliable overlay recovery", "Highlights", "Faster startup", "Clearer recovery states", "Keyboard-first controls", "Got it", "Read full notes")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing release-notes rendering copy: {missing}")
if text.count("✓") != 3:
    raise SystemExit("release notes card must show three highlights")
print("overlay release notes card rendering contract passed")
