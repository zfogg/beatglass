from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-display-card.svg").read_text(encoding="utf-8")

assert "overlay-display-card.svg" in docs
for marker in ("Studio Monitor · selected", "Available", "Projector", "Unavailable · choose another"):
    assert marker in svg, marker
print("overlay display card rendering contract: ok")
