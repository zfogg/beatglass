from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-startup-restored-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay ready", "saved native window state", "Click-through and 75% opacity", "Continue"):
        assert text in svg, text
    assert "overlay-startup-restored-card.svg" in gallery
    print("overlay startup restored rendering contract passed")
if __name__ == "__main__": main()
