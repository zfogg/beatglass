from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-microcopy-card.svg").read_text(encoding="utf-8")

assert "overlay-microcopy-card.svg" in gallery
for marker in ("Overlay recovery pending", "Persisted display", "Review settings", "Review recovery"):
    assert marker in svg, marker
print("overlay microcopy card rendering contract: ok")
