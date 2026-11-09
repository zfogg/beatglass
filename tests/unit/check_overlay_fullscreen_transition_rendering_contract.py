from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-fullscreen-transition-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Updating fullscreen behavior", "Fullscreen state detected", "settings remain preserved", "Cancel"): assert text in svg, text
    assert "overlay-fullscreen-transition-card.svg" in gallery
    print("overlay fullscreen transition rendering contract passed")
if __name__ == "__main__": main()
