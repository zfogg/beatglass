from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-resume-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Resume monitoring", "Built-in Speakers are ready", "same layout", "No settings need to be changed", "On resume", "Audio capture starts again", "Waveform and FFT", "Esc to keep paused")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing resume rendering copy: {missing}")
if text.count("✓") != 2:
    raise SystemExit("resume card must show two resume guarantees")
print("overlay resume card rendering contract passed")
