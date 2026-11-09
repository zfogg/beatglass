from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-layout-card.svg").read_text(encoding="utf-8")

assert "overlay-layout-card.svg" in gallery
for marker in ("Layout hierarchy", "RECOVERY PENDING", "Retained display", "Next action", "24px rhythm"):
    assert marker in svg, marker
print("overlay layout card rendering contract: ok")
