from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-banner-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Safe mode · Native overlay paused", "Your session is still running", "Playback, capture, and library indexing continue", "Settings"): assert text in svg, text
    assert "overlay-native-safe-mode-banner-card.svg" in gallery
    print("overlay native safe mode banner rendering contract passed")
if __name__ == "__main__": main()
