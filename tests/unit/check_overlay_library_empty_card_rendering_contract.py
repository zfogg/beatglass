from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-empty-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("No tracks found", "Try another search", "Esc to clear", "Your library is ready for more", "Add a folder", "discover tracks", "Close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing empty-library rendering copy: {missing}")
if "#718096" not in text or "#56d38b" not in text:
    raise SystemExit("empty library card must show neutral state and add action")
print("overlay library empty card rendering contract passed")
