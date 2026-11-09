from pathlib import Path
ROOT = Path(__file__).parents[2]
GALLERY = ROOT / "docs" / "overlay-design-gallery.md"
def main() -> None:
    gallery = GALLERY.read_text(encoding="utf-8")
    specs = {
        "overlay-library-remove-folder-confirm-card.svg": ("Remove this library folder?", "Remove folder", "Nothing is deleted from disk", "#b84758"),
        "overlay-library-folder-removed-card.svg": ("Folder removed", "Your files were not changed", "Add folder", "#56d38b"),
    }
    for filename, required in specs.items():
        svg = (ROOT / "docs" / filename).read_text(encoding="utf-8")
        missing = [text for text in required if text not in svg]
        assert not missing, f"{filename} missing rendering contract text: {missing}"
        assert filename in gallery
    print("overlay library remove-folder rendering contract passed")
if __name__ == "__main__": main()
