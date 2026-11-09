from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-settings-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Native overlay settings", "Safe mode · native overlay paused", "Grant access", "Saved theme, layout, opacity"): assert text in svg, text
    assert "overlay-native-safe-mode-settings-card.svg" in gallery
    print("overlay native safe mode settings rendering contract passed")
if __name__ == "__main__": main()
