from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-rename-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Rename this view", "Compact speakers", "24 / 40", "Saved settings", "Top right · Compact", "only the label", "Save name", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing rename rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("rename card must distinguish save and cancel actions")
print("overlay rename card rendering contract passed")
