from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-qualification-matrix.svg").read_text(encoding="utf-8")

assert "overlay-qualification-matrix.svg" in gallery
for marker in ("Ready overlay", "Recovery pending", "Disconnected", "Linux", "macOS", "Windows"):
    assert marker in svg, marker
print("overlay qualification matrix contract: ok")
