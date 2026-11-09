from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-settings-restored-card.svg": ("Settings restored", "Click-through and display selection", "Continue", "Review settings", "#7ff0c4"), "overlay-settings-reset-card.svg": ("Settings reset", "safe defaults", "Undo reset", "audio files and library were not affected", "#ffd36e")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay settings persistence rendering contract passed")
if __name__ == "__main__": main()
