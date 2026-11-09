from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-layout-control-card.svg": ("Overlay layout", "POSITION", "Compact · 360 px", "Apply layout", "#56d38b"), "overlay-layout-applied-card.svg": ("Layout updated", "Bottom · Compact", "Settings saved locally", "Change", "#7ff0c4")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay layout control rendering contract passed")
if __name__ == "__main__": main()
