from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-quick-actions-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Quick actions", "Show / hide overlay", "Unlock interactions", "Open controls", "Pause monitoring", "View all shortcuts", "Ctrl + Shift + H")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing quick-actions rendering copy: {missing}")
if text.count("Ctrl + Shift +") < 4:
    raise SystemExit("quick-actions card must expose keyboard parity for each action")
print("overlay quick actions card rendering contract passed")
