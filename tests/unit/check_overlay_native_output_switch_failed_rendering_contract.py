from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-output-switch-failed-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Display unavailable", "Switch was not applied", "Choose display", "Keep Display 2", "capture continues"):
        assert text in svg, f"missing switch failure copy: {text}"
    assert "overlay-native-output-switch-failed-card.svg" in gallery
    print("overlay native output switch failure rendering contract passed")


if __name__ == "__main__":
    main()
