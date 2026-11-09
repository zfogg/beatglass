from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-click-through-control-card.svg": ("Mouse interaction", "Click-through mode", "unlock shortcut", "Apply", "#56d38b"), "overlay-click-through-applied-card.svg": ("Click-through enabled", "Ctrl + Shift + U", "Hold the shortcut", "Change", "#7ff0c4")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay click-through rendering contract passed")
if __name__ == "__main__": main()
