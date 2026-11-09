from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-live-controls-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Output healthy", "Hide overlay", "Open settings", "Safe mode", "audio capture continues"):
        assert text in svg, f"missing live control copy: {text}"
    assert "overlay-native-live-controls-card.svg" in gallery
    print("overlay native live controls rendering contract passed")


if __name__ == "__main__":
    main()
