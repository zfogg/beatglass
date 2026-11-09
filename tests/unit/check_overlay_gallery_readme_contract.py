from pathlib import Path


readme = Path("README.md").read_text(encoding="utf-8")
assert "docs/overlay-design-gallery.md" in readme
assert "native screenshot qualification states" in readme
print("overlay gallery README contract: ok")
