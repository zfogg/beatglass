from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-tray-handoff-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Overlay is in the tray", "Session retained", "Show overlay", "Keep hidden", "does not restart capture"):
        assert text in svg, f"missing tray handoff copy: {text}"
    assert "overlay-native-tray-handoff-card.svg" in gallery
    print("overlay native tray handoff rendering contract passed")


if __name__ == "__main__":
    main()
