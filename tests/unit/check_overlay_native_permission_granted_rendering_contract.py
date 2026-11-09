from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-permission-granted-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay permission granted", "Native overlay available", "audio files remain untouched", "Continue"): assert text in svg, text
    assert "overlay-native-permission-granted-card.svg" in gallery
    print("overlay native permission granted rendering contract passed")
if __name__ == "__main__": main()
