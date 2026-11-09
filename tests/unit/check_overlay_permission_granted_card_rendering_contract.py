from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-permission-granted-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Audio access ready", "Permission granted", "Waveform and FFT", "live analysis", "layout and settings are unchanged", "Next step", "Start playback", "Show overlay", "Open controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing granted rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("permission granted card must expose success and next actions")
print("overlay permission granted card rendering contract passed")
