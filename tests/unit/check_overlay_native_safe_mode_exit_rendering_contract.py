from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-exit-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Resume native overlay?", "Settings saved while safe mode", "Resume overlay", "Stay safe"): assert text in svg, text
    assert "overlay-native-safe-mode-exit-card.svg" in gallery
    print("overlay native safe mode exit rendering contract passed")
if __name__ == "__main__": main()
