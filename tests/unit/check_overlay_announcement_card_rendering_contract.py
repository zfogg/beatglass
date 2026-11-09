from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-announcement-card.svg").read_text(encoding="utf-8")

assert "overlay-announcement-card.svg" in docs
for marker in ("Recovery announcement", "persisted settings retained", "revision 12", "Review recovery", "Accessible.name"):
    assert marker in svg, marker
print("overlay announcement card rendering contract: ok")
