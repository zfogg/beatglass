from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-complete-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Library is ready", "480 tracks ready", "Waveform and FFT", "Overlay monitoring was not interrupted", "Find something to play", "Search the library", "Keep listening")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing complete-library rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("complete library card must expose success and continuation actions")
print("overlay library complete card rendering contract passed")
