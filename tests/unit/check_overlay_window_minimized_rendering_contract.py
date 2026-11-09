from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-window-minimized-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Overlay minimized", "Display · theme · opacity · position", "Restore", "No settings are changed"): assert text in svg, text
    assert "overlay-window-minimized-card.svg" in gallery
    print("overlay window minimized rendering contract passed")
if __name__ == "__main__": main()
