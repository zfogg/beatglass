from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-loading-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Loading library", "overlay stays available", "Indexing Music", "128 of 480 tracks", "What you can do now", "Keep listening", "No audio capture is interrupted", "Show overlay", "Run later")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing loading-library rendering copy: {missing}")
if "stroke-dasharray" not in text or "#56d38b" not in text:
    raise SystemExit("loading library card must show progress and safe live state")
print("overlay library loading card rendering contract passed")
