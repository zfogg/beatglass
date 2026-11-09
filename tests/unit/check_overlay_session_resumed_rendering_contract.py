from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-session-resumed-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Session resumed", "last saved native state", "Click-through and tray activation", "Continue"): assert text in svg, text
    assert "overlay-session-resumed-card.svg" in gallery
    print("overlay session resumed rendering contract passed")
if __name__ == "__main__": main()
