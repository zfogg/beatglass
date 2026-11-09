from pathlib import Path
ROOT=Path(__file__).parents[2]
def main():
    svg=(ROOT/"docs/overlay-session-resume-failed-card.svg").read_text(encoding="utf-8"); gallery=(ROOT/"docs/overlay-design-gallery.md").read_text(encoding="utf-8")
    for text in ("Could not resume session", "library and audio files are safe", "No saved state was changed", "Retry", "Settings"): assert text in svg, text
    assert "overlay-session-resume-failed-card.svg" in gallery
    print("overlay session resume failed rendering contract passed")
if __name__ == "__main__": main()
