from pathlib import Path


gallery = Path("docs/overlay-design-gallery.md").read_text(encoding="utf-8")
assets = list(Path("docs").glob("overlay-*.svg"))

assert len(assets) >= 18
for asset in assets:
    assert asset.name in gallery, asset.name
assert "screenshot qualification" in gallery.lower()
print(f"overlay design gallery contract: ok ({len(assets)} renderings)")
