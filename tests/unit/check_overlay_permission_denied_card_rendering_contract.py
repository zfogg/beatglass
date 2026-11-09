from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-permission-denied-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Audio access is off", "Permission denied", "Waveform and FFT are paused", "layout is retained", "To continue", "Open system settings", "Open settings", "Try again")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing denied rendering copy: {missing}")
if "#d96d70" not in text or "#56d38b" not in text:
    raise SystemExit("permission denied card must distinguish error and recovery actions")
print("overlay permission denied card rendering contract passed")
