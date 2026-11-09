from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-interaction-locked-card.svg": ("Overlay controls locked", "Ctrl + Shift + U", "Unlock", "#7db4ff"), "overlay-interaction-unlocked-card.svg": ("Overlay controls unlocked", "controls active", "Lock now", "#7ff0c4")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay interaction lock rendering contract passed")
if __name__ == "__main__": main()
