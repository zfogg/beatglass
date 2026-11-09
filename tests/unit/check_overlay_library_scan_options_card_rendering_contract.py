from pathlib import Path

CARD = Path(__file__).parents[2] / "docs" / "overlay-library-scan-options-card.svg"
text = CARD.read_text(encoding="utf-8")
required = ("Scan options", "All supported audio", "Recommended", "Only new files", "keep existing metadata", "During a scan", "Monitoring continues", "Start scan", "Cancel")
missing = [value for value in required if value not in text]
if missing:
    raise SystemExit(f"missing scan-options rendering copy: {missing}")
if "#56d38b" not in text or "#718096" not in text:
    raise SystemExit("scan options card must distinguish selected and alternate modes")
print("overlay library scan options card rendering contract passed")
