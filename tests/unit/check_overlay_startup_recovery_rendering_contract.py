from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-startup-recovery-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Restoring overlay", "saved native window state", "Display 1 · Midnight · Bottom", "Cancel"):
        assert text in svg, text
    assert "overlay-startup-recovery-card.svg" in gallery
    print("overlay startup recovery rendering contract passed")
if __name__ == "__main__": main()
