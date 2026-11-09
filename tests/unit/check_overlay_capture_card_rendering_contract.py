from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-capture-card.svg").read_text(encoding="utf-8")

assert "overlay-capture-card.svg" in docs
for marker in ("Output capture", "REACQUIRING", "Studio Monitor", "Retry monitoring"):
    assert marker in svg, marker
print("overlay capture card rendering contract: ok")
