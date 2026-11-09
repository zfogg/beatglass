from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-reduced-motion-card.svg").read_text(encoding="utf-8")

assert "overlay-reduced-motion-card.svg" in gallery
for marker in ("Motion preference", "Reduced motion", "Static visualization", "Keep static view"):
    assert marker in svg, marker
print("overlay reduced-motion card rendering contract: ok")
