from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-display-restore-applied-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Overlay restored", "Healthy output", "0 dropped frames", "Open overlay", "settings were restored"):
        assert text in svg, f"missing restore-applied copy: {text}"
    assert "overlay-native-display-restore-applied-card.svg" in gallery
    print("overlay native display restore applied rendering contract passed")


if __name__ == "__main__":
    main()
