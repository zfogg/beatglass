from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-window-restored-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay restored", "Theme, opacity, and position", "Done"): assert text in svg, text
    assert "overlay-window-restored-card.svg" in gallery
    print("overlay window restored rendering contract passed")
if __name__ == "__main__": main()
