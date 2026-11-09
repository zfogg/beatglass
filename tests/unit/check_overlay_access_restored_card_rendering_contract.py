from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-access-restored-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Access restored", "Permission restored", "Built-in Speakers are available", "layout and selected output are unchanged", "Ready to resume", "Start playback", "Show overlay", "Open controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing restored rendering copy: {missing}")
if "#56d38b" not in text or "#2a3444" not in text:
    raise SystemExit("restored card must expose success and next actions")
print("overlay access restored card rendering contract passed")
