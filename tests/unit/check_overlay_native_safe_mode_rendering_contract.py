from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Safe mode active", "Playback · capture · library", "Grant native permission", "Settings", "Continue"): assert text in svg, text
    assert "overlay-native-safe-mode-card.svg" in gallery
    print("overlay native safe mode rendering contract passed")
if __name__ == "__main__": main()
