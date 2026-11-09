from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-theme-card.svg").read_text(encoding="utf-8")

assert "overlay-theme-card.svg" in gallery
for marker in ("Dark · selected", "Light", "Contrast", "AAA targets"):
    assert marker in svg, marker
print("overlay theme card rendering contract: ok")
