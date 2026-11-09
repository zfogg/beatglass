from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg = (ROOT / "docs" / "overlay-window-chrome-control-card.svg").read_text(encoding="utf-8")
    gallery = (ROOT / "docs" / "overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Window chrome", "Borderless overlay", "Native controls stay available", "Apply"):
        assert text in svg, text
    assert "overlay-window-chrome-control-card.svg" in gallery
    print("overlay window chrome rendering contract passed")
if __name__ == "__main__": main()
