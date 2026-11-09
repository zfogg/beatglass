from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-fft-card.svg").read_text(encoding="utf-8")

assert "overlay-fft-card.svg" in docs
for marker in ("FFT spectrum", "CAPTURE READY", "Peak: 440 Hz", "note A4"):
    assert marker in svg, marker
print("overlay FFT card rendering contract: ok")
