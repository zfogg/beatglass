from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-always-on-top-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Always on top", "Always on top enabled", "Click-through and tray controls", "Apply"):
        assert text in svg, text
    assert "overlay-always-on-top-card.svg" in gallery
    print("overlay always-on-top rendering contract passed")
if __name__ == "__main__": main()
