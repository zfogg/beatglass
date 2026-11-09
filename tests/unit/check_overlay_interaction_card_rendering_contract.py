from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-interaction-card.svg").read_text(encoding="utf-8")

assert "overlay-interaction-card.svg" in docs
for marker in ("CLICK-THROUGH ENABLED", "Unlock overlay", "Pointer input passes through"):
    assert marker in svg, marker
print("overlay interaction card rendering contract: ok")
