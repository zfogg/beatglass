from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-opacity-card.svg").read_text(encoding="utf-8")

assert "overlay-opacity-card.svg" in docs
for marker in ("Current value", "90%", "20% minimum", "100% maximum", "screen-reader value"):
    assert marker in svg, marker
print("overlay opacity card rendering contract: ok")
