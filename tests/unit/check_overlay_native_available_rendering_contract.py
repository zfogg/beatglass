from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-available-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Native overlay available", "live analysis", "saved appearance", "Activate"): assert text in svg, text
    assert "overlay-native-available-card.svg" in gallery
    print("overlay native available rendering contract passed")
if __name__ == "__main__": main()
