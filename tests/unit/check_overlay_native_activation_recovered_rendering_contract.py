from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-activation-recovered-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Activation recovered", "native overlay is visible again", "Theme, position, opacity", "Done"): assert text in svg, text
    assert "overlay-native-activation-recovered-card.svg" in gallery
    print("overlay native activation recovered rendering contract passed")
if __name__ == "__main__": main()
