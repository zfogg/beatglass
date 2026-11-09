from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-activating-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Activating native overlay", "Display 1 · Bottom · Click-through", "Restoring theme, opacity, and position", "Cancel"): assert text in svg, text
    assert "overlay-native-activating-card.svg" in gallery
    print("overlay native activating rendering contract passed")
if __name__ == "__main__": main()
