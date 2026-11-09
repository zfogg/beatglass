from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-recovery-disabled-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Automatic recovery off", "Manual recovery only", "Enable auto recovery", "Keep manual", "not capture"):
        assert text in svg, f"missing recovery-disabled copy: {text}"
    assert "overlay-native-recovery-disabled-card.svg" in gallery
    print("overlay native recovery disabled rendering contract passed")


if __name__ == "__main__":
    main()
