from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-unavailable-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Native overlay unavailable", "Playback · capture · library", "No audio files, tags, or settings were modified", "Settings", "Continue"): assert text in svg, text
    assert "overlay-native-unavailable-card.svg" in gallery
    print("overlay native unavailable rendering contract passed")
if __name__ == "__main__": main()
