from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-focus-card.svg").read_text(encoding="utf-8")

assert "overlay-focus-card.svg" in gallery
for marker in ("Keyboard focus", "Review recovery", "4px focus ring", "Tab order"):
    assert marker in svg, marker
print("overlay focus card rendering contract: ok")
