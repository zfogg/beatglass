from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-search-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Find a track", "Search title, artist, or folder", "Ctrl + L", "1 RESULT", "Midnight Drive", "Neon Avenue", "Selected track", "Open track", "Esc to close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing library-search rendering copy: {missing}")
if "#56d38b" not in text:
    raise SystemExit("library search card must expose selected result and action")
print("overlay library search card rendering contract passed")
