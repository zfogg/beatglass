from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-duplicates-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Duplicate tracks found", "Your files are unchanged", "12 possible duplicate tracks", "Review matches", "Keep all", "#56d38b")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-duplicates-card.svg" in gallery
    print("overlay library duplicates card rendering contract passed")
if __name__ == "__main__": main()
