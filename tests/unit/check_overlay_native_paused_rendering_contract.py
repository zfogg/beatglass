from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-paused-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Monitoring paused", "Resume monitoring", "No input is captured", "Open settings", "remain unchanged"):
        assert text in svg, f"missing paused-state copy: {text}"
    assert "overlay-native-paused-card.svg" in gallery
    print("overlay native paused rendering contract passed")


if __name__ == "__main__":
    main()
