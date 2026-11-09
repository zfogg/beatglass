from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-fullscreen-behavior-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Fullscreen behavior", "Stay visible", "live analysis visible", "Apply"):
        assert text in svg, text
    assert "overlay-fullscreen-behavior-card.svg" in gallery
    print("overlay fullscreen behavior rendering contract passed")
if __name__ == "__main__": main()
