"""Verify no enumerated displays require display selection recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplaySelectionRequired = window.overlayBridge.screenNames.length === 0 || savedIndex < 0'
if marker not in qml:
    raise SystemExit("Missing no-display selection requirement")
print("Overlay no-display selection contract verified (1 check)")
