from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-recovery-timeline.svg").read_text(encoding="utf-8")

assert "overlay-recovery-timeline.svg" in docs
for marker in ("Display lost", "Review", "Retry / restore", "Ready"):
    assert marker in svg, marker
print("overlay recovery timeline rendering contract: ok")
