from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-scan-scheduled-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Library scan scheduled", "Tomorrow at 03:00", "Only changed files", "Scan now", "Close", "#7ff0c4")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-scan-scheduled-card.svg" in gallery
    print("overlay library scan scheduled card rendering contract passed")
if __name__ == "__main__": main()
