from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-scan-confirm-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Scan your library now?", "Music / DJ Library", "about 2 minutes", "Start scan", "Not now", "cancel safely", "#56d38b")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-scan-confirm-card.svg" in gallery
    print("overlay library scan confirm card rendering contract passed")
if __name__ == "__main__": main()
