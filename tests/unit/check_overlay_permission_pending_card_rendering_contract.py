from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-permission-pending-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Checking audio access", "Permission check in progress", "overlay will remain visible", "No audio is captured", "While waiting", "layout and selected output", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing pending rendering copy: {missing}")
if "stroke-dasharray" not in text or "#e3b45b" not in text:
    raise SystemExit("pending card must show an in-progress state")
print("overlay permission pending card rendering contract passed")
