from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-activation-retrying-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Retrying overlay activation", "ATTEMPT 2 OF 3", "session remains available", "Cancel"): assert text in svg, text
    assert "overlay-native-activation-retrying-card.svg" in gallery
    print("overlay native activation retrying rendering contract passed")
if __name__ == "__main__": main()
