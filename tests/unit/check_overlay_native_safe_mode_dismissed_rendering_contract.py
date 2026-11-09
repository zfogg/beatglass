from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-dismissed-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Safe mode dismissed", "Native overlay paused", "Playback, capture, and library are unaffected", "Settings"): assert text in svg, text
    assert "overlay-native-safe-mode-dismissed-card.svg" in gallery
    print("overlay native safe mode dismissed rendering contract passed")
if __name__ == "__main__": main()
