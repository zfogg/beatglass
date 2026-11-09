from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-session-ended-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Session ended", "saved your overlay settings", "Your audio files and library were not changed", "Close"): assert text in svg, text
    assert "overlay-session-ended-card.svg" in gallery
    print("overlay session ended rendering contract passed")
if __name__ == "__main__": main()
