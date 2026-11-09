from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-ready-card.svg").read_text(encoding="utf-8")

assert "overlay-ready-card.svg" in docs
for marker in ("READY", "Studio Monitor", "Review summary", "Hide overlay", "Persisted display"):
    assert marker in svg, marker
print("overlay ready card rendering contract: ok")
