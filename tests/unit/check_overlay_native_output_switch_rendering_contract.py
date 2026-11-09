from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-output-switch-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Switch output?", "Display 2 → Display 1", "Switch output", "Cancel", "Capture pauses briefly"):
        assert text in svg, f"missing output switch copy: {text}"
    assert "overlay-native-output-switch-card.svg" in gallery
    print("overlay native output switch rendering contract passed")


if __name__ == "__main__":
    main()
