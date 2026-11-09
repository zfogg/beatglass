from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-display-none-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("No display available", "Overlay safely retained", "Refresh displays", "Keep monitoring", "Saved placement"):
        assert text in svg, f"missing no-display copy: {text}"
    assert "overlay-native-display-none-card.svg" in gallery
    print("overlay native display none rendering contract passed")


if __name__ == "__main__":
    main()
