from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-folder-unavailable-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Library folder unavailable", "indexed tracks remain available", "128 tracks available", "Choose folder", "Keep offline", "#ff7f86")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-folder-unavailable-card.svg" in gallery
    print("overlay library folder unavailable card rendering contract passed")
if __name__ == "__main__": main()
