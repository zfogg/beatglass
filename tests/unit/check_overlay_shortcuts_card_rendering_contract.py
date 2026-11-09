from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-shortcuts-card.svg").read_text(encoding="utf-8")

assert "overlay-shortcuts-card.svg" in docs
for marker in ("Ctrl+Alt+W", "Ctrl+Alt+J", "Ctrl+Alt+R", "Review recovery"):
    assert marker in svg, marker
print("overlay shortcuts card rendering contract: ok")
