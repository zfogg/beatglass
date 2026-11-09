from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-error-card.svg").read_text(encoding="utf-8")

assert "overlay-error-card.svg" in gallery
for marker in ("ACTION NEEDED", "Overlay could not be restored", "settings are retained", "Retry overlay"):
    assert marker in svg, marker
print("overlay error card rendering contract: ok")
