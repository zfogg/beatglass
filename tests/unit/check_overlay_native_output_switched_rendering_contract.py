from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-output-switched-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Output switched", "Live on Display 1", "Capture resumed", "Open overlay", "presentation was preserved"):
        assert text in svg, f"missing output success copy: {text}"
    assert "overlay-native-output-switched-card.svg" in gallery
    print("overlay native output switched rendering contract passed")


if __name__ == "__main__":
    main()
