from pathlib import Path
SVG = Path(__file__).parents[2] / "docs" / "overlay-library-scan-schedule-disabled-card.svg"
GALLERY = Path(__file__).parents[2] / "docs" / "overlay-design-gallery.md"
def main() -> None:
    svg = SVG.read_text(encoding="utf-8"); gallery = GALLERY.read_text(encoding="utf-8")
    required = ("Automatic scans are off", "Manual scans only", "No background indexing", "Scan now", "Enable later", "#56d38b")
    missing = [text for text in required if text not in svg]
    assert not missing, f"missing rendering contract text: {missing}"
    assert "overlay-library-scan-schedule-disabled-card.svg" in gallery
    print("overlay library scan schedule disabled card rendering contract passed")
if __name__ == "__main__": main()
