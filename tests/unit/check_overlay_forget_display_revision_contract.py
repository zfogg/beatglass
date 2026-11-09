"""Verify forgetting a saved display advances overlay settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayPreferredScreenIndex = -1\n        window.overlaySettingsRevision += 1'
if marker not in qml:
    raise SystemExit("Missing forget-display settings revision update")
print("Overlay forget-display revision contract verified (1 check)")
