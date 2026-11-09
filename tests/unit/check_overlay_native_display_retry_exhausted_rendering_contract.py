from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-display-retry-exhausted-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Safe mode is active", "three attempts", "Monitoring is still available", "Open settings", "Dismiss"):
        assert text in svg, f"missing exhausted-retry copy: {text}"
    assert "overlay-native-display-retry-exhausted-card.svg" in gallery
    print("overlay native display retry exhausted rendering contract passed")


if __name__ == "__main__":
    main()
