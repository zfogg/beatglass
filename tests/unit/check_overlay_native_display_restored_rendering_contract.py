from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-display-restored-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Display restored", "Restore overlay", "Keep safe view", "Monitoring continues", "settings are preserved"):
        assert text in svg, f"missing restored-display copy: {text}"
    assert "overlay-native-display-restored-card.svg" in gallery
    print("overlay native display restored rendering contract passed")


if __name__ == "__main__":
    main()
