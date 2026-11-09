from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-recovery-complete-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Recovery complete", "Theme and layout preserved", "Click-through, opacity", "Continue"): assert text in svg, text
    assert "overlay-recovery-complete-card.svg" in gallery
    print("overlay recovery complete rendering contract passed")
if __name__ == "__main__": main()
