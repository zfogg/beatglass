from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-import-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Bring in a saved view", "beatglass-compact.json", "Valid", "This will change", "selected output", "This will not change", "Audio files", "Import view", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing import rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("import card must distinguish valid/import and cancel actions")
print("overlay import card rendering contract passed")
