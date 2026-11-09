from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-permission-revoked-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay permission revoked", "Library and audio are unaffected", "Grant access", "Dismiss"): assert text in svg, text
    assert "overlay-native-permission-revoked-card.svg" in gallery
    print("overlay native permission revoked rendering contract passed")
if __name__ == "__main__": main()
