from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-activated-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Native overlay activated", "Live analysis is visible", "Theme, opacity, position", "Done"): assert text in svg, text
    assert "overlay-native-activated-card.svg" in gallery
    print("overlay native activated rendering contract passed")
if __name__ == "__main__": main()
