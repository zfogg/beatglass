from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-status-popover-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Beatglass status", "Monitoring normally", "Open overlay", "Pause monitoring", "Click-through on"):
        assert text in svg, f"missing status popover copy: {text}"
    assert "overlay-native-status-popover-card.svg" in gallery
    print("overlay native status popover rendering contract passed")


if __name__ == "__main__":
    main()
