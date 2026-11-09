from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-activation-shortcut-card.svg": ("Activation shortcut", "Ctrl + Shift + B", "Available", "Change", "#56d38b"), "overlay-activation-shortcut-applied-card.svg": ("Shortcut saved", "No conflicts detected", "Try the shortcut now", "Done", "#7ff0c4")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay activation shortcut rendering contract passed")
if __name__ == "__main__": main()
