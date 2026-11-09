from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-access-revoked-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Access changed", "Live analysis paused", "Waveform and FFT", "layout and selected output are retained", "Restore access", "Open system settings", "Open settings", "Dismiss")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing revoked rendering copy: {missing}")
if "#d96d70" not in text or "#56d38b" not in text:
    raise SystemExit("revoked card must distinguish error and recovery actions")
print("overlay access revoked card rendering contract passed")
