from pathlib import Path
ROOT = Path(__file__).parents[2]
def main() -> None:
    svg=(ROOT/"docs/overlay-fullscreen-auto-hide-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Auto-hide enabled", "returns when fullscreen ends", "Apply"): assert text in svg, text
    assert "overlay-fullscreen-auto-hide-card.svg" in gallery
    print("overlay fullscreen auto-hide rendering contract passed")
if __name__ == "__main__": main()
