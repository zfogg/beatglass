from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-activation-confirm-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Overlay is ready", "Activate overlay", "Not now", "Click-through is on"):
        assert text in svg, f"missing activation copy: {text}"
    assert "overlay-native-activation-confirm-card.svg" in gallery
    print("overlay native activation confirmation rendering contract passed")


if __name__ == "__main__":
    main()
