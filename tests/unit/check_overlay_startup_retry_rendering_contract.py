from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-startup-retry-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Could not restore overlay", "No settings were changed", "Retry", "Settings"): assert text in svg, text
    assert "overlay-startup-retry-card.svg" in gallery
    print("overlay startup retry rendering contract passed")
if __name__ == "__main__": main()
