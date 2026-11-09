from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-scan-canceled-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Scan canceled", "completed metadata is safe", "128 tracks remain indexed", "No files were changed", "You can continue later", "Resume scanning", "Resume scan", "Close")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing canceled-scan rendering copy: {missing}")
if "#718096" not in text or "#56d38b" not in text:
    raise SystemExit("canceled scan card must show neutral state and resume action")
print("overlay library scan canceled card rendering contract passed")
