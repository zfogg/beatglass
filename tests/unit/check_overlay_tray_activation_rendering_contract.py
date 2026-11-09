from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-tray-activation-card.svg": ("Overlay controls", "Overlay visible", "Hide overlay", "Open settings", "#56d38b"), "overlay-tray-hidden-card.svg": ("Overlay hidden", "Ready to restore", "Show overlay", "settings are preserved", "#7ff0c4")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay tray activation rendering contract passed")
if __name__ == "__main__": main()
