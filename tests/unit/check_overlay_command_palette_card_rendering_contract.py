from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-command-palette-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("What do you want to do?", "Type a command", "SHOW OVERLAY", "OPEN CONTROLS", "RUN DIAGNOSTICS", "Navigate", "Select", "Close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing command palette rendering copy: {missing}")
if text.count("Enter") < 4:
    raise SystemExit("command palette must expose keyboard selection affordances")
print("overlay command palette card rendering contract passed")
