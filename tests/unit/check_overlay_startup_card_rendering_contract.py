from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-startup-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Getting things ready", "Settings loaded", "Startup checklist", "Overlay window created", "Output selected", "Ready for live audio", "Show overlay")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing startup rendering copy: {missing}")
if text.count("✓") != 3:
    raise SystemExit("startup card must show three completed checklist steps")
print("overlay startup card rendering contract passed")
