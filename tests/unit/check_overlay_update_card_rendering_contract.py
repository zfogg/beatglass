from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-update-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("An update is ready", "Beatglass 0.2", "Improved overlay recovery", "Your settings will be preserved", "Choose when to update", "Install now", "Later", "Release notes")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing update rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("update card must distinguish install and defer actions")
print("overlay update card rendering contract passed")
