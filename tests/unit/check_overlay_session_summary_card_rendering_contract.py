from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-session-summary-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Session summary", "SESSION TIME", "42:18", "TRACKS", "analyzed locally", "HEALTH", "0 interruptions", "Session settings", "Open details")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing session summary rendering copy: {missing}")
if "local only" not in text or "#56d38b" not in text:
    raise SystemExit("session summary must communicate local scope and health")
print("overlay session summary card rendering contract passed")
