"""Verify successful display recovery advances overlay settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayPreferredDisplayName = window.overlayBridge.screenNames[savedIndex]\n        window.overlaySettingsRevision += 1'
if marker not in qml:
    raise SystemExit("Missing display recovery revision update")
print("Overlay recovery revision contract verified (1 check)")
