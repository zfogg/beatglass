from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-control-panel-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Control panel", "Opacity", "Position", "Interaction", "Click-through", "Apply changes", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing control panel rendering copy: {missing}")
if "fill=\"#56d38b\"" not in text:
    raise SystemExit("control panel must expose a positive action accent")
print("overlay control panel card rendering contract passed")
