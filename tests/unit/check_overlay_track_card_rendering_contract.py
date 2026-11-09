from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-track-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Now playing", "Midnight Drive", "Neon Avenue", "04:12 remaining", "Listening and analyzing", "Analysis context", "Open waveform", "local metadata only")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing track rendering copy: {missing}")
if "Waveform, FFT, and timing" not in text:
    raise SystemExit("track card must connect metadata with analysis context")
print("overlay track card rendering contract passed")
