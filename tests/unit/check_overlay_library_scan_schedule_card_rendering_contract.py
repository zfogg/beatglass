from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-scan-schedule-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Library scan schedule", "Automatic scans", "Every 24 hours", "Only changed files", "Save schedule", "#56d38b")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-scan-schedule-card.svg" in gallery
    print("overlay library scan schedule card rendering contract passed")
if __name__ == "__main__": main()
