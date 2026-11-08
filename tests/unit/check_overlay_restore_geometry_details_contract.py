"""Verify persisted restore status exposes native geometry details."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayRestoreState"',
    'window.overlayBridge.geometry.width + "×" + window.overlayBridge.geometry.height',
    '"geometry unavailable"',
    'window.overlayBridge.displayName',
    'Math.round(window.overlayBridge.opacity * 100)',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing restore geometry markers: " + ", ".join(missing))
print(f"Overlay restore geometry details contract verified ({len(required)} checks)")
