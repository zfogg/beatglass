from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-native-safe-mode-resume-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Resume native overlay", "Display 2 · Midnight · Bottom", "Click-through, opacity", "Resume overlay", "Stay safe"): assert text in svg, text
    assert "overlay-native-safe-mode-resume-card.svg" in gallery
    print("overlay native safe mode resume rendering contract passed")
if __name__ == "__main__": main()
