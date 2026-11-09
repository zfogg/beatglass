from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-activation-blocked-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay activation blocked", "Saved settings are unchanged", "Playback, capture, and library indexing", "Retry", "Settings"): assert text in svg, text
    assert "overlay-native-activation-blocked-card.svg" in gallery
    print("overlay native activation blocked rendering contract passed")
if __name__ == "__main__": main()
