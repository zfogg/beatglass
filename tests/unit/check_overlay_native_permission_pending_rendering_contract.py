from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-permission-pending-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay permission needed", "Visible analysis during playback", "does not modify your audio files", "Allow", "Not now"): assert text in svg, text
    assert "overlay-native-permission-pending-card.svg" in gallery
    print("overlay native permission pending rendering contract passed")
if __name__ == "__main__": main()
