from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-tips-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Make it yours", "Try Compact mode", "Use keyboard shortcuts", "Pin a favorite view", "Check diagnostics", "Show tips on startup", "Done")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing tips rendering copy: {missing}")
if text.count("TIP 0") != 4:
    raise SystemExit("tips card must show four numbered tips")
print("overlay tips card rendering contract passed")
