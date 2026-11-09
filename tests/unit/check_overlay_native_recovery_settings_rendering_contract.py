from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-recovery-settings-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Overlay recovery", "PREFERRED DISPLAY", "Try automatically", "Save recovery", "Current monitoring continues"):
        assert text in svg, f"missing recovery settings copy: {text}"
    assert "overlay-native-recovery-settings-card.svg" in gallery
    print("overlay native recovery settings rendering contract passed")


if __name__ == "__main__":
    main()
