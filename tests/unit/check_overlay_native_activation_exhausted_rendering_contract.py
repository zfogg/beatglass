from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-activation-exhausted-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Activation paused", "Your session is still running", "Playback, capture, and library state are unaffected", "Settings", "Continue"): assert text in svg, text
    assert "overlay-native-activation-exhausted-card.svg" in gallery
    print("overlay native activation exhausted rendering contract passed")
if __name__ == "__main__": main()
