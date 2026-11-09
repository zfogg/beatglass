from pathlib import Path


status = Path("docs/platform-qualification-status.md").read_text(encoding="utf-8")
assert "docs/overlay-design-gallery.md" in status
assert "do not change any platform row to READY" in status
print("overlay qualification status gallery contract: ok")
