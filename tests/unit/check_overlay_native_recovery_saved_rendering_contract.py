from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-recovery-saved-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Recovery saved", "Automatic safe-mode recovery", "Return to overlay", "No restart is needed", "settings are retained"):
        assert text in svg, f"missing recovery-saved copy: {text}"
    assert "overlay-native-recovery-saved-card.svg" in gallery
    print("overlay native recovery saved rendering contract passed")


if __name__ == "__main__":
    main()
