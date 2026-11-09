from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SVG = ROOT / "docs" / "overlay-native-recovery-enabled-card.svg"
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"


def main() -> None:
    svg = SVG.read_text(encoding="utf-8")
    gallery = GALLERY.read_text(encoding="utf-8")
    for text in ("Automatic recovery on", "Safe fallback enabled", "Return to overlay", "retries are bounded", "capture and local analysis"):
        assert text in svg, f"missing recovery-enabled copy: {text}"
    assert "overlay-native-recovery-enabled-card.svg" in gallery
    print("overlay native recovery enabled rendering contract passed")


if __name__ == "__main__":
    main()
