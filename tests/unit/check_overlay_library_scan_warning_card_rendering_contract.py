from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-scan-warning-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Scan completed with warnings", "214 tracks indexed · 3 skipped", "No files were changed", "Review warnings", "Done", "#ffd36e")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-scan-warning-card.svg" in gallery
    print("overlay library scan warning card rendering contract passed")
if __name__ == "__main__": main()
