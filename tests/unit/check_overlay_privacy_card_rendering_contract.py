from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-privacy-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Your audio stays yours", "Live analysis", "No file changes", "never writes tags", "Local by default", "Read privacy notes")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing privacy rendering copy: {missing}")
if text.count("#56d38b") < 3:
    raise SystemExit("privacy card must show each local-safety guarantee")
print("overlay privacy card rendering contract passed")
