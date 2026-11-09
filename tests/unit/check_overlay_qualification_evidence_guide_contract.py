from pathlib import Path


guide = Path("docs/overlay-qualification-evidence.md").read_text(encoding="utf-8")
for marker in ("overlay-ready-card.svg", "overlay-settings-card.svg", "overlay-disconnected-card.svg", "overlay-interaction-card.svg", "overlay-capture-card.svg", "READY"):
    assert marker in guide, marker
assert "cannot mark a" in guide and "platform `READY`" in guide
print("overlay qualification evidence guide contract: ok")
