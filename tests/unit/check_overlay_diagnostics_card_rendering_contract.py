from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-diagnostics-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("System diagnostics", "Overlay window", "Audio capture", "Permissions", "Healthy", "Connected", "Review recommended", "Run check again")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing diagnostics rendering copy: {missing}")
if "#56d38b" not in text or "#e3b45b" not in text:
    raise SystemExit("diagnostics card must distinguish healthy and review states")
print("overlay diagnostics card rendering contract passed")
