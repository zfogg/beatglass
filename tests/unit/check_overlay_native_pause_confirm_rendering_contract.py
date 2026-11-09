from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-pause-confirm-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Pause monitoring?", "Pause now", "Keep running", "Resume from the tray", "No audio files or tags"):
        assert text in svg, f"missing pause confirmation copy: {text}"
    assert "overlay-native-pause-confirm-card.svg" in gallery
    print("overlay native pause confirmation rendering contract passed")


if __name__ == "__main__":
    main()
