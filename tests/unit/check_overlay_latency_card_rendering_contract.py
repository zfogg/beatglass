from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-latency-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Signal performance", "INPUT LATENCY", "12 ms", "SAMPLE RATE", "48 kHz", "DROPPED FRAMES", "Healthy monitoring path", "Open diagnostics")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing latency rendering copy: {missing}")
if text.count("#56d38b") < 3:
    raise SystemExit("latency card must expose healthy status accents")
print("overlay latency card rendering contract passed")
