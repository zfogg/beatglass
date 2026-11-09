from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-add-folder-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Add a music folder", "C:\\Users\\You\\Music", "Ready to scan", "What happens next", "supported audio", "local waveform/FFT metadata", "never moves", "Add folder", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing add-folder rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("add-folder card must distinguish scan and cancel actions")
print("overlay library add-folder card rendering contract passed")
