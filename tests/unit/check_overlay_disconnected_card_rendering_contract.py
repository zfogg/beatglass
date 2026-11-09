from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-disconnected-card.svg").read_text(encoding="utf-8")

assert "overlay-disconnected-card.svg" in docs
for marker in ("DISPLAY UNAVAILABLE", "Reconnect a display", "Retry overlay", "Persisted settings remain safe"):
    assert marker in svg, marker
print("overlay disconnected card rendering contract: ok")
