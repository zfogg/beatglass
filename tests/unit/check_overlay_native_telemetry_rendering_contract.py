from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-telemetry-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Overlay diagnostics", "Healthy", "3 ms", "0 dropped frames", "Open diagnostics"):
        assert text in svg, f"missing telemetry copy: {text}"
    assert "overlay-native-telemetry-card.svg" in gallery
    print("overlay native telemetry rendering contract passed")


if __name__ == "__main__":
    main()
