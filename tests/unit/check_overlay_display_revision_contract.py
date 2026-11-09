"""Verify successful display selection advances the settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayPreferredDisplayName = name\n        window.overlaySettingsRevision += 1'
if marker not in qml:
    raise SystemExit("Missing overlay display settings revision update")
print("Overlay display revision contract verified (1 check)")
