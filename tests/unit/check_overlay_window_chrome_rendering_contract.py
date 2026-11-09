from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-window-chrome.svg").read_text(encoding="utf-8")

assert "overlay-window-chrome.svg" in docs
for marker in ("BEATGLASS", "LIVE READY", "128 BPM", "FFT spectrum", "Output capture active"):
    assert marker in svg, marker
print("overlay window chrome rendering contract: ok")
