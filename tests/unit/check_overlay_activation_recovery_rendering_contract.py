from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {"overlay-activation-pending-card.svg": ("Showing overlay", "Restoring your saved overlay state", "Cancel", "#ffd36e"), "overlay-activation-failed-card.svg": ("Overlay could not open", "No settings were changed", "Retry", "Display settings", "#ff7f86")}
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay activation recovery rendering contract passed")
if __name__ == "__main__": main()
