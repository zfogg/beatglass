"""Verify geometry status accessibility text reflects ready and fallback states."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayGeometryStatus"',
    'Accessible.description: window.overlayBridge && window.overlayBridge.geometry',
    '"Persisted overlay position and size: " + text',
    '"Persisted overlay position and size unavailable; control-window fallback remains active."',
    'Accessible.role: Accessible.Status',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing geometry accessibility markers: " + ", ".join(missing))
print(f"Overlay geometry accessibility contract verified ({len(required)} checks)")
