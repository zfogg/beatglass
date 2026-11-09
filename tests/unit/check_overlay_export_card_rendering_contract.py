from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-export-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Save this view", "Compact speakers", "Ready to save", "Included in this view", "selected output", "Always kept local", "Audio files", "Save view", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing export rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("export card must distinguish save and cancel actions")
print("overlay export card rendering contract passed")
