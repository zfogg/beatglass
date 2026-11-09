from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-settings-card.svg").read_text(encoding="utf-8")

assert "overlay-settings-card.svg" in docs
for marker in ("RECOVERY PENDING", "Review recovery", "Restore overlay", "Settings retained"):
    assert marker in svg, marker
print("overlay settings card rendering contract: ok")
