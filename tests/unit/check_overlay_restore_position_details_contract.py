"""Verify persisted restore status exposes native position and dimensions."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayRestoreState"',
    'window.overlayBridge.geometry.x + "," + window.overlayBridge.geometry.y',
    'window.overlayBridge.geometry.width + "×" + window.overlayBridge.geometry.height',
    '"geometry unavailable"',
    'window.overlayBridge.displayName',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing restore position markers: " + ", ".join(missing))
print(f"Overlay restore position details contract verified ({len(required)} checks)")
