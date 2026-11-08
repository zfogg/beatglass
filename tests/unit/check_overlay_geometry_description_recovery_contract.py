"""Verify geometry description announces pending recovery when needed."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayGeometryStatus"',
    'Accessible.description: window.overlayBridge && window.overlayBridge.geometry',
    '"Persisted overlay position and size: " + text',
    'window.overlayDisplayRecoveryPending ? ". Recovery is pending." : "."',
    '"Persisted overlay position and size unavailable; control-window fallback remains active."',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing geometry description recovery markers: " + ", ".join(missing))
print(f"Overlay geometry description recovery contract verified ({len(required)} checks)")
