from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-startup-fallback-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Startup display unavailable", "saved appearance preserved", "Choose display", "Continue"):
        assert text in svg, text
    assert "overlay-startup-fallback-card.svg" in gallery
    print("overlay startup fallback rendering contract passed")
if __name__ == "__main__": main()
