from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-display-picker-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Choose display", "Display 2", "Display 1", "Available", "Apply display"):
        assert text in svg, f"missing display picker copy: {text}"
    assert "overlay-native-display-picker-card.svg" in gallery
    print("overlay native display picker rendering contract passed")


if __name__ == "__main__":
    main()
