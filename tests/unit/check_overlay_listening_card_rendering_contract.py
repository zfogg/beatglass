from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-listening-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Listening now", "Built-in Speakers", "48 kHz", "Live analysis", "Waveform and frequency views", "0 dropped frames", "Pause", "Open controls")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing listening rendering copy: {missing}")
if "Monitoring is healthy" not in text or "#56d38b" not in text:
    raise SystemExit("listening card must communicate healthy live analysis")
print("overlay listening card rendering contract passed")
