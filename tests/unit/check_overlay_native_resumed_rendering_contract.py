from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-resumed-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Monitoring resumed", "Native overlay restored", "Capture is live", "Open overlay", "previous display"):
        assert text in svg, f"missing resumed-state copy: {text}"
    assert "overlay-native-resumed-card.svg" in gallery
    print("overlay native resumed rendering contract passed")


if __name__ == "__main__":
    main()
