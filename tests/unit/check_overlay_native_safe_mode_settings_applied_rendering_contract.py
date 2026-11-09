from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-settings-applied-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Settings applied", "next native activation", "Safe mode remains active", "Done"): assert text in svg, text
    assert "overlay-native-safe-mode-settings-applied-card.svg" in gallery
    print("overlay native safe mode settings applied rendering contract passed")
if __name__ == "__main__": main()
