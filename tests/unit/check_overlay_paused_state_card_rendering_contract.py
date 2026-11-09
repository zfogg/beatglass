from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-paused-state-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Monitoring paused", "settings are safe", "No audio is being captured", "Nothing will be written", "Resume behavior", "Resume monitoring", "Open controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing paused-state rendering copy: {missing}")
if "#e3b45b" not in text or "#56d38b" not in text:
    raise SystemExit("paused state must distinguish paused status from resume action")
print("overlay paused state card rendering contract passed")
