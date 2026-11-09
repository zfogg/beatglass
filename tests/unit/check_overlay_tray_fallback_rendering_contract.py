from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-tray-fallback.svg").read_text(encoding="utf-8")

assert "overlay-tray-fallback.svg" in docs
for marker in ("MONITORING ACTIVE", "Live output monitoring remains available", "Review recovery", "Retry overlay"):
    assert marker in svg, marker
print("overlay tray fallback rendering contract: ok")
