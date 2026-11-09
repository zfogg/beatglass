from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-permission-denied-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay permission denied", "Playback and capture continue", "library, audio files, and saved settings are safe", "Settings", "Dismiss"): assert text in svg, text
    assert "overlay-native-permission-denied-card.svg" in gallery
    print("overlay native permission denied rendering contract passed")
if __name__ == "__main__": main()
