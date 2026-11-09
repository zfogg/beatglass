from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-display-retry-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Restoring overlay", "Attempt 2 of 3", "Try now", "Stop retrying", "safe mode"):
        assert text in svg, f"missing display retry copy: {text}"
    assert "overlay-native-display-retry-card.svg" in gallery
    print("overlay native display retry rendering contract passed")


if __name__ == "__main__":
    main()
