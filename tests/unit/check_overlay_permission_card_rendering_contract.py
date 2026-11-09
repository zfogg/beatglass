from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-permission-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Allow audio access", "System audio permission", "waveform and frequency", "system settings", "What Beatglass can access", "no file writes", "Open settings", "Not now")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing permission rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("permission card must distinguish access and defer actions")
print("overlay permission card rendering contract passed")
