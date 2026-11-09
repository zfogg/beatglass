from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-fullscreen-restored-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay restored", "Fullscreen ended", "saved theme and opacity", "Done"): assert text in svg, text
    assert "overlay-fullscreen-restored-card.svg" in gallery
    print("overlay fullscreen restored rendering contract passed")
if __name__ == "__main__": main()
