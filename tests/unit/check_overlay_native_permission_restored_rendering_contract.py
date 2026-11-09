from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-permission-restored-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay permission restored", "Display and appearance preserved", "Library and audio state were never interrupted", "Resume"): assert text in svg, text
    assert "overlay-native-permission-restored-card.svg" in gallery
    print("overlay native permission restored rendering contract passed")
if __name__ == "__main__": main()
