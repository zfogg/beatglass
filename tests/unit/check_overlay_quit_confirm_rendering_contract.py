from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-quit-confirm-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Quit Beatglass?", "Current settings will be saved", "Quit", "Keep running"): assert text in svg, text
    assert "overlay-quit-confirm-card.svg" in gallery
    print("overlay quit confirmation rendering contract passed")
if __name__ == "__main__": main()
