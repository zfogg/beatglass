from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-live-status-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Live status", "CURRENT OUTPUT", "Open controls", "Signal health", "Hide overlay", "Change output", "Last refreshed")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing live status rendering copy: {missing}")
if "fill=\"#56d38b\"" not in text:
    raise SystemExit("live status card must expose a positive health accent")
print("overlay live status card rendering contract passed")
