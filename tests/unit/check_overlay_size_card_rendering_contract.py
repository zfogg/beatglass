from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-size-card.svg").read_text(encoding="utf-8")

assert "overlay-size-card.svg" in docs
for marker in ("Compact", "Medium · selected", "Large", "250 × 110", "310 × 130", "430 × 180"):
    assert marker in svg, marker
print("overlay size card rendering contract: ok")
