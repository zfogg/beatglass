from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-visualization-card.svg").read_text(encoding="utf-8")

assert "overlay-visualization-card.svg" in docs
for marker in ("Waveform", "CAPTURE READY", "No waveform sample selected", "Start monitoring"):
    assert marker in svg, marker
print("overlay visualization card rendering contract: ok")
