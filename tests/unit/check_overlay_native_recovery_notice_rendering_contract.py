from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-recovery-notice-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Overlay recovery available", "Keep working", "Monitoring continues", "Open settings", "Dismiss"):
        assert text in svg, f"missing recovery notice copy: {text}"
    assert "overlay-native-recovery-notice-card.svg" in gallery
    print("overlay native recovery notice rendering contract passed")


if __name__ == "__main__":
    main()
